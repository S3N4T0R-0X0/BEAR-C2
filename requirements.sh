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
        # shellcheck disable=SC1091
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

echo
echo "[*] Updating pip / setuptools / wheel..."

"$PYTHON" -m pip install \
    --break-system-packages \
    --upgrade \
    pip \
    setuptools \
    wheel

# ------------------------------------------------
# Install / update dependencies
# ------------------------------------------------

echo
echo "[*] Installing/updating dependencies..."

"$PYTHON" -m pip install \
    --break-system-packages \
    --upgrade \
    customtkinter \
    Pillow \
    pyperclip \
    requests \
    Flask \
    Werkzeug \
    aioquic \
    pycryptodome \
    cryptography \
    Telethon \
    discord.py \
    aiohttp \
    pyinstaller \
    donut-shellcode \

# ------------------------------------------------
# Verify dependencies
# ------------------------------------------------

echo
echo "[*] Verifying dependencies..."

"$PYTHON" - <<'PY'
import importlib

modules = [
    "customtkinter",
    "PIL",
    "pyperclip",
    "requests",
    "flask",
    "werkzeug",
    "aioquic",
    "Crypto",
    "cryptography",
    "telethon",
    "discord",
    "aiohttp",
    "donut",

]

failed = []

for module in modules:
    try:
        importlib.import_module(module)
        print(f"[+] {module}: OK")
    except Exception as exc:
        print(f"[-] {module}: FAILED")
        print(f"    {exc}")
        failed.append(module)

if failed:
    print()
    print("[!] Failed imports:")
    for module in failed:
        print(f"    - {module}")
    raise SystemExit(1)

print()
print("[+] All third-party dependencies verified.")
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
