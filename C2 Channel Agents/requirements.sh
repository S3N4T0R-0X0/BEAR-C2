#!/usr/bin/env bash
set -euo pipefail

echo "=============================================="
echo "       Evil Agents - Dependencies"
echo "=============================================="

sudo apt update

sudo apt install -y \
    mingw-w64 \
    g++-mingw-w64-x86-64 \
    binutils-mingw-w64-x86-64

echo
echo "[+] Evil Agents dependencies installed."
echo
echo "Required Windows components:"
echo "  - Windows API"
echo "  - WinHTTP"
echo "  - GDI+"
echo "  - CryptoAPI"
echo "  - Winsock"
echo "  - C++17 Standard Library"
