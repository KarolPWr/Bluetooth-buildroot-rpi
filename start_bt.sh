#!/bin/bash

set -e


modprobe hci_uart
rfcomm
bluetoothd

hciattach /dev/ttyS0 bcm43xx 921600 noflow -

sleep 2

hciconfig hci0 up
