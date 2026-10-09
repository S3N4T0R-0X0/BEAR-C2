#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ------------------------------------------------
# Check Rust & C++ build requirements
# ------------------------------------------------

RUST_FAIL=0
CPP_FAIL=0

command -v cargo  >/dev/null 2>&1 || RUST_FAIL=1
command -v rustc  >/dev/null 2>&1 || RUST_FAIL=1
command -v rustup >/dev/null 2>&1 || RUST_FAIL=1

if command -v rustup >/dev/null 2>&1; then
    rustup target list --installed 2>/dev/null | grep -q "x86_64-pc-windows-gnu" || RUST_FAIL=1
fi

command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || CPP_FAIL=1
command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 || CPP_FAIL=1


if [[ $CPP_FAIL -eq 1 ]]; then
    echo "[*] C++ (MinGW-w64) is missing – installing..."
    if command -v apt >/dev/null 2>&1; then
        sudo apt update
        sudo apt install -y mingw-w64
    elif command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y mingw64-gcc-c++
    elif command -v pacman >/dev/null 2>&1; then
        sudo pacman -Sy --noconfirm mingw-w64-gcc
    elif command -v zypper >/dev/null 2>&1; then
        sudo zypper install -y mingw64-cross-gcc-c++
    else
        echo "[!] Could not detect package manager. Install MinGW-w64 manually."
        exit 1
    fi
    CPP_FAIL=0
fi


if [[ $RUST_FAIL -eq 1 ]]; then
    echo "[*] Rust is missing – installing..."

    if ! command -v rustup >/dev/null 2>&1; then
        curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y

        source "$HOME/.cargo/env"
    fi


    if ! rustup target list --installed 2>/dev/null | grep -q "x86_64-pc-windows-gnu"; then
        rustup target add x86_64-pc-windows-gnu
    fi

    RUST_FAIL=0
fi

echo "[+] Rust and C++ are ready."

PYTHON="${PYTHON:-python3}"
APP="${APP:-BEAR-C2.py}"

echo "=============================================="
echo "        Python Build & Dependency Setup"
echo "=============================================="

# ------------------------------------------------
# Check Python
# ------------------------------------------------

if ! command -v "$PYTHON" >/dev/null 2>&1; then
    echo "[!] Python 3 not found."
    exit 1
fi

echo "[+] Python: $($PYTHON --version)"

# ------------------------------------------------
# Update pip / build tools
# ------------------------------------------------

# ------------------------------------------------
# Update pip / build tools (silent)
# ------------------------------------------------

echo
echo "[*] Updating pip / setuptools / wheel..."

"$PYTHON" -m pip install \
    --quiet \
    --break-system-packages \
    --disable-pip-version-check \
    --no-warn-script-location \
    --upgrade \
    pip setuptools wheel >/dev/null 2>&1 || true

# ------------------------------------------------
# Install only MISSING dependencies (silent)
# ------------------------------------------------

echo
echo "[*] Checking dependencies..."

"$PYTHON" - <<'PY'
import importlib.util
import subprocess
import sys


REQUIRED = [
    ("customtkinter", "customtkinter"),
    ("PIL",           "Pillow"),
    ("pyperclip",     "pyperclip"),
    ("requests",      "requests"),
    ("flask",         "Flask"),
    ("werkzeug",      "Werkzeug"),
    ("aioquic",       "aioquic"),
    ("Crypto",        "pycryptodome"),
    ("cryptography",  "cryptography"),
    ("telethon",      "Telethon"),
    ("discord",       "discord.py"),
    ("aiohttp",       "aiohttp"),
    ("donut",         "donut-shellcode"),

]

missing_pkgs = []

for module, pkg in REQUIRED:
    if importlib.util.find_spec(module) is None:
        print(f"[*] Missing: {pkg}")
        missing_pkgs.append(pkg)
    else:
        print(f"[+] Found:   {pkg}")


if importlib.util.find_spec("PyInstaller") is None:
    print("[*] Missing: pyinstaller")
    missing_pkgs.append("pyinstaller")
else:
    print("[+] Found:   pyinstaller")

if not missing_pkgs:
    print()
    print("[+] All dependencies already installed.")
    sys.exit(0)

print()
print(f"[*] Installing {len(missing_pkgs)} missing package(s)...")

cmd = [
    sys.executable, "-m", "pip", "install",
    "--quiet",
    "--break-system-packages",
    "--disable-pip-version-check",
    "--no-warn-script-location",
    *missing_pkgs,
]

result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

if result.returncode != 0:
    print("[!] pip failed. Re-running with output for debugging...")
    subprocess.run(cmd)
    sys.exit(1)

print("[+] Missing dependencies installed.")
PY

# ------------------------------------------------
# Check application
# ------------------------------------------------

echo
echo "[*] Checking application..."

if [[ ! -f "$APP" ]]; then
    echo "[!] $APP not found."
    exit 1
fi

# ------------------------------------------------
# Syntax check
# ------------------------------------------------

echo "[*] Running syntax check..."

"$PYTHON" -m py_compile "$APP"

echo "[+] Syntax check passed."

# ------------------------------------------------
# Check image folder (now inside Stagers-Loaders)
# ------------------------------------------------

IMAGE_DIR="Stagers-Loaders/image"
if [[ -d "$IMAGE_DIR" ]]; then
    echo "[+] Image folder '$IMAGE_DIR' found – will be bundled."
else
    echo "[!] Warning: Image folder '$IMAGE_DIR' not found – the executable may lack visual assets."
fi

# ------------------------------------------------
# Clean previous build
# ------------------------------------------------

echo
echo "[*] Cleaning previous PyInstaller output..."

rm -rf build dist

# ------------------------------------------------
# Build executable (bundle image folder)
# ------------------------------------------------

echo
echo "[*] Building executable with embedded resources..."

"$PYTHON" -m PyInstaller \
    --onefile \
    --clean \
    --add-data "Stagers-Loaders/image:Stagers-Loaders/image" \
    --add-data "Reaper Node Payloads:Reaper Node Payloads" \
    "$APP"

# ------------------------------------------------
# Copy executable
# ------------------------------------------------

EXECUTABLE="${APP%.py}"

echo
echo "[*] Checking build result..."

if [[ ! -f "dist/$EXECUTABLE" ]]; then
    echo "[!] PyInstaller did not produce:"
    echo "    dist/$EXECUTABLE"
    exit 1
fi

cp "dist/$EXECUTABLE" "$SCRIPT_DIR/"

chmod +x "$SCRIPT_DIR/$EXECUTABLE"

# ------------------------------------------------
# Cleanup generated build artifacts
# ------------------------------------------------

echo
echo "[*] Cleaning build artifacts..."

rm -rf build
rm -rf dist
rm -rf __pycache__
rm -rf "$IMAGE_DIR"          
rm -f "${APP%.py}.spec"
rm -f BEAR-C2.py

# ------------------------------------------------
# Finished
# ------------------------------------------------

echo
echo "=============================================="
echo "[+] Build completed successfully."
echo "=============================================="
echo
echo "[+] Executable:"
echo "    $SCRIPT_DIR/$EXECUTABLE"
echo
echo "[+] Permissions:"
ls -lh "$SCRIPT_DIR/$EXECUTABLE"
echo
echo "=============================================="
echo "[+] Done"
echo "=============================================="
