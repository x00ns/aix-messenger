#!/bin/bash

set -e

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
