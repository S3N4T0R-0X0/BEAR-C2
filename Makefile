#!/usr/bin/env bash

set -Eeuo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

success() {
    printf "${GREEN}[+] %s${NC}\n" "$1"
}

warn() {
    printf "${RED}[!] %s${NC}\n" "$1"
}

info() {
    printf "${BLUE}[*] %s${NC}\n" "$1"
}

trap 'warn "Command failed at line $LINENO. Aborting."; exit 1' ERR

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ------------------------------------------------
# Fix dpkg state
# ------------------------------------------------

info "Fixing dpkg state (if needed)..."

if sudo dpkg --configure -a; then
    success "dpkg configuration completed."
else
    warn "dpkg configuration failed. Continuing anyway..."
fi

if sudo apt --fix-broken install -y; then
    success "Broken package check completed."
else
    warn "apt --fix-broken install failed. Continuing anyway..."
fi

# ------------------------------------------------
# Update 
# ------------------------------------------------

echo
info "Updating..."

if sudo apt update; then
    success "Repository lists updated successfully."
else
    warn "apt update failed. Continuing anyway..."
fi

echo
info "Continuing with build requirements..."

# ------------------------------------------------
# Check Rust & C++ build requirements
# ------------------------------------------------

RUST_FAIL=0
CPP_FAIL=0

command -v cargo  >/dev/null 2>&1 || RUST_FAIL=1
command -v rustc  >/dev/null 2>&1 || RUST_FAIL=1
command -v rustup >/dev/null 2>&1 || RUST_FAIL=1

if command -v rustup >/dev/null 2>&1; then
    if ! rustup target list --installed 2>/dev/null |
        grep -q "x86_64-pc-windows-gnu"; then
        RUST_FAIL=1
    fi
fi

command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || CPP_FAIL=1
command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 || CPP_FAIL=1

# ------------------------------------------------
# Install C++ requirements
# ------------------------------------------------

if [[ "$CPP_FAIL" -eq 1 ]]; then
    info "MinGW-w64 is missing. Installing..."

    if command -v apt >/dev/null 2>&1; then
        sudo apt install -y mingw-w64
    elif command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y mingw64-gcc-c++
    elif command -v pacman >/dev/null 2>&1; then
        sudo pacman -Sy --noconfirm mingw-w64-gcc
    elif command -v zypper >/dev/null 2>&1; then
        sudo zypper install -y mingw64-cross-gcc-c++
    else
        warn "Package manager not detected. Install MinGW-w64 manually."
        exit 1
    fi
fi

CPP_FAIL=0

command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 || CPP_FAIL=1
command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1 || CPP_FAIL=1

if [[ "$CPP_FAIL" -ne 0 ]]; then
    warn "MinGW-w64 installation failed or required compilers are missing."
    exit 1
fi

success "MinGW-w64 compilers are ready."

# ------------------------------------------------
# Install Rust requirements
# ------------------------------------------------

if [[ "$RUST_FAIL" -eq 1 ]]; then
    info "Rust or the required Windows target is missing."

    if ! command -v rustup >/dev/null 2>&1; then
        if ! command -v curl >/dev/null 2>&1; then
            warn "curl is missing. Install curl before installing Rust."
            exit 1
        fi

        info "Installing Rust using the official installer..."

        curl --proto '=https' --tlsv1.2 -sSf \
            https://sh.rustup.rs | sh -s -- -y

        if [[ -f "$HOME/.cargo/env" ]]; then
            source "$HOME/.cargo/env"
        fi
    fi

    if ! command -v rustup >/dev/null 2>&1; then
        warn "rustup is still unavailable after installation."
        exit 1
    fi

    if ! rustup target list --installed 2>/dev/null |
        grep -q "x86_64-pc-windows-gnu"; then
        info "Installing Rust Windows GNU target..."
        rustup target add x86_64-pc-windows-gnu
    fi
fi

if ! command -v cargo >/dev/null 2>&1 ||
   ! command -v rustc >/dev/null 2>&1 ||
   ! command -v rustup >/dev/null 2>&1; then
    warn "Rust toolchain verification failed."
    exit 1
fi

if ! rustup target list --installed 2>/dev/null |
    grep -q "x86_64-pc-windows-gnu"; then
    warn "Required Rust Windows target is not installed."
    exit 1
fi

success "Rust and C++ requirements are ready."

# ------------------------------------------------
# Python configuration
# ------------------------------------------------

PYTHON="${PYTHON:-python3}"
APP="${APP:-BEAR-C2.py}"

echo
echo "=============================================="
echo "        Python Build & Dependency Setup"
echo "=============================================="

if ! command -v "$PYTHON" >/dev/null 2>&1; then
    warn "Python 3 was not found."
    exit 1
fi

success "Python: $("$PYTHON" --version)"

# ------------------------------------------------
# Pin setuptools < 82 to keep pkg_resources.NullProvider
# (PyInstaller runtime hook needs it)
# ------------------------------------------------

echo
info "Installing setuptools<82 (required by PyInstaller)..."
"$PYTHON" -m pip install \
    --quiet \
    --break-system-packages \
    --disable-pip-version-check \
    --no-warn-script-location \
    "setuptools<82" 2>/dev/null || true

# ------------------------------------------------
# Update pip / build tools
# ------------------------------------------------

echo
info "Updating pip and wheel..."

if "$PYTHON" -m pip install \
    --quiet \
    --break-system-packages \
    --disable-pip-version-check \
    --no-warn-script-location \
    --upgrade pip wheel; then
    success "pip and wheel updated."
else
    warn "Updating pip/wheel failed. Continuing anyway..."
fi

# ------------------------------------------------
# Verify NullProvider is available
# ------------------------------------------------

echo
info "Verifying pkg_resources.NullProvider availability..."

if "$PYTHON" - <<'PY'
import sys
try:
    import pkg_resources
    if hasattr(pkg_resources, "NullProvider"):
        print("[+] pkg_resources.NullProvider is available.")
        sys.exit(0)
    else:
        print("[!] pkg_resources is present but NullProvider is missing.")
        sys.exit(1)
except Exception as e:
    print(f"[!] pkg_resources import failed: {e}")
    sys.exit(1)
PY
then
    success "NullProvider check passed."
else
    warn "setuptools is too new. Installing a compatible version..."
    "$PYTHON" -m pip install \
        --break-system-packages \
        --disable-pip-version-check \
        --force-reinstall \
        "setuptools<82"

    if "$PYTHON" -c "import pkg_resources; assert hasattr(pkg_resources, 'NullProvider')" 2>/dev/null; then
        success "setuptools rolled back; NullProvider is now available."
    else
        warn "Failed to obtain a compatible setuptools. PyInstaller build will fail."
        exit 1
    fi
fi

# ------------------------------------------------
# Check Python dependencies
# ------------------------------------------------

echo
info "Checking Python dependencies..."

if ! "$PYTHON" - <<'PY'
import importlib.util
import subprocess
import sys

REQUIRED = [
    ("customtkinter", "customtkinter"),
    ("PIL", "Pillow"),
    ("pyperclip", "pyperclip"),
    ("requests", "requests"),
    ("flask", "Flask"),
    ("werkzeug", "Werkzeug"),
    ("aioquic", "aioquic"),
    ("Crypto", "pycryptodome"),
    ("cryptography", "cryptography"),
    ("telethon", "Telethon"),
    ("discord", "discord.py"),
    ("aiohttp", "aiohttp"),
    ("donut", "donut-shellcode"),
    ("PyInstaller", "pyinstaller"),
]

missing_pkgs = []

for module, package in REQUIRED:
    try:
        found = importlib.util.find_spec(module) is not None
    except (ImportError, ValueError, ModuleNotFoundError):
        found = False

    if found:
        print(f"\033[0;32m[+] Found: {package}\033[0m")
    else:
        print(f"\033[0;31m[!] Missing: {package}\033[0m")
        missing_pkgs.append(package)

if not missing_pkgs:
    print("\033[0;32m[+] All dependencies are installed.\033[0m")
    sys.exit(0)

print(f"\n[*] Installing {len(missing_pkgs)} missing package(s)...")

cmd = [
    sys.executable, "-m", "pip", "install",
    "--break-system-packages",
    "--disable-pip-version-check",
    "--no-warn-script-location",
    *missing_pkgs,
]

result = subprocess.run(cmd)

if result.returncode != 0:
    print("\033[0;31m[!] Dependency installation failed.\033[0m")
    sys.exit(1)

print("\033[0;32m[+] Missing dependencies installed.\033[0m")
PY
then
    warn "Python dependency setup failed."
    exit 1
fi

# ------------------------------------------------
# Re-verify NullProvider after dependency install
# (a dependency may have pulled in a newer setuptools)
# ------------------------------------------------

echo
info "Re-verifying pkg_resources.NullProvider after dependency install..."

if ! "$PYTHON" -c "import pkg_resources; assert hasattr(pkg_resources, 'NullProvider')" 2>/dev/null; then
    warn "A dependency upgraded setuptools. Rolling back to a compatible version..."
    "$PYTHON" -m pip install \
        --break-system-packages \
        --disable-pip-version-check \
        --force-reinstall \
        "setuptools<82"

    if ! "$PYTHON" -c "import pkg_resources; assert hasattr(pkg_resources, 'NullProvider')" 2>/dev/null; then
        warn "Could not restore a compatible setuptools."
        exit 1
    fi
fi

success "NullProvider is available; ready to build."

# ------------------------------------------------
# Check application
# ------------------------------------------------

echo
info "Checking application..."

if [[ ! -f "$APP" ]]; then
    warn "Application file '$APP' was not found."
    exit 1
fi

success "Application file found: $APP"

# ------------------------------------------------
# Syntax check
# ------------------------------------------------

info "Running Python syntax check..."

if "$PYTHON" -m py_compile "$APP"; then
    success "Python syntax check passed."
else
    warn "Python syntax check failed."
    exit 1
fi

# ------------------------------------------------
# Check image directory
# ------------------------------------------------

IMAGE_DIR="Resources/image"

if [[ -d "$IMAGE_DIR" ]]; then
    success "Image directory found: $IMAGE_DIR"
else
    warn "Image directory '$IMAGE_DIR' is missing."
fi

# ------------------------------------------------
# Check previous build output
# ------------------------------------------------

echo
info "Preparing build output..."

if [[ -d build ]]; then
    rm -rf build
    success "Previous build directory removed."
fi

if [[ -d dist ]]; then
    rm -rf dist
    success "Previous distribution directory removed."
fi

# ------------------------------------------------
# Build application
# ------------------------------------------------

echo
info "Starting PyInstaller build..."

if [[ -d "$IMAGE_DIR" ]]; then
    "$PYTHON" -m PyInstaller \
        --onefile \
        --clean \
            --add-data "Resources/image:Resources/image" \
            --add-data "Payloads:Payloads" \
        "$APP"
else
    "$PYTHON" -m PyInstaller \
        --onefile \
        --clean \
        --add-data "Payloads:Payloads" \
        "$APP"
fi

# ------------------------------------------------
# Verify build result
# ------------------------------------------------

EXECUTABLE="${APP%.py}"

echo
info "Checking build result..."

if [[ ! -f "dist/$EXECUTABLE" ]]; then
    warn "Expected executable was not produced: dist/$EXECUTABLE"
    exit 1
fi

if cp "dist/$EXECUTABLE" "$SCRIPT_DIR/$EXECUTABLE"; then
    success "Executable copied to project directory."
else
    warn "Failed to copy executable."
    exit 1
fi

if chmod +x "$SCRIPT_DIR/$EXECUTABLE"; then
    success "Executable permissions configured."
else
    warn "Failed to set executable permissions."
    exit 1
fi

# ------------------------------------------------
# Cleanup generated build artifacts
# ------------------------------------------------

echo
info "Cleaning build artifacts..."

rm -rf build
rm -rf dist
rm -rf __pycache__
rm -rf "$IMAGE_DIR"
rm -f "${APP%.py}.spec"
rm -f BEAR-C2.py

success "Build artifacts and source files cleaned."

# ------------------------------------------------
# Finished
# ------------------------------------------------

echo
echo "=============================================="
success "Build completed successfully."
echo "=============================================="
echo
success "Executable:"
printf '%s\n' "$SCRIPT_DIR/$EXECUTABLE"
echo
success "File details:"
ls -lh "$SCRIPT_DIR/$EXECUTABLE"
echo
echo "=============================================="
success "Done"
echo "=============================================="
