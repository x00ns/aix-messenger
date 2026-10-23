#!/bin/bash

set -e

# make clean

BINARY="/usr/local/bin/aix"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found."
    exit 1;
fi

echo "<<< Uninstalling binary package >>>"
sudo rm -f /usr/local/bin/aix
