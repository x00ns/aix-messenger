#!/bin/bash

set -e

echo " <<< Installing a aix-messenger >>>"
make clean && make

BINARY="./aix"

if [ ! -f "$BINARY" ]; then
    echo "ERROR: Binary not found. Please recompile source code"
fi

echo "<<< Installing binary package >>>"
sudo mkdir -p /usr/bin/aix-messenger
sudo cp "$BINARY" /usr/bin/aix-messenger/aix
sudo chmod +x /usr/bin/aix-messenger/aix

if [ -d /run/systemd/system ] || pidof systemd >/dev/null; then
    echo "[ Systemd detected! ]"
    mkdir -p "$HOME/.config/systemd/user"

    cp init/aix-daemon.service "$HOME/.config/systemd/user"
    # reload daemon
    systemctl --user daemon-reload
    systemctl --user enable --now aix-daemon.service
    systemctl --user status aix-daemon.service --no-pager
fi
