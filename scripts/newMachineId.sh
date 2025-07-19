#!/bin/bash
sudo rm -f /etc/machine-id
sudo systemd-machine-id-setup
echo "Machine Id Restablecido"
echo "Acordate de reestablecer la MAC adress para que te queden IPs Distintas!" 