#!/bin/bash

set -e

echo " <<< Installing a aix-messenger >>>"
make clean && make

BINARY="./aix"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found. Please recompile source code"
    exit 1;
fi

echo "<<< Installing binary package >>>"
mkdir -p "$HOME/.aix-messenger/"
sudo cp "$BINARY" /usr/bin/aix
sudo chmod +x /usr/bin/aix
