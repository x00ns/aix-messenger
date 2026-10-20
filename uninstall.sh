#!/bin/bash

set -e

# make clean

BINARY="./aix_client"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found."
    exit 1;
fi

echo "<<< Uninstalling binary package >>>"
rm -rf aix_server
sudo rm -f /usr/local/bin/aix
