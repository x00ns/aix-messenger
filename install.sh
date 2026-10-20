#!/bin/bash

set -e

if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
fi

echo "=================================="
echo "  Let's installing aix-messenger"
echo "      Your distro: $NAME ($OS)    "
echo "=================================="

case "$OS" in
    "ubuntu" | "debian")
        echo "Installing depencies via apt..."
        sudo apt update
        sudo apt install build-essential libssl-dev libsqlite3-dev -y
        ;;
    "arch")
        echo "Installing depencies via pacman..."
        sudo pacman -S base-devel sqlite openssl
        ;;
esac

echo " <<< Installing a aix-messenger >>>"
make clean && make

BINARY="./aix_client"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found. Please recompile source code"
    exit 1;
fi

echo "<<< Installing binary package >>>"
rm -rf aix_server
mkdir -p "$HOME/.aix-messenger/"
sudo cp "$BINARY" /usr/local/bin/aix
sudo chmod +x /usr/local/bin/aix
