# Bluetooth-buildroot-rpi
Image and scripts for buildroot+rpi+ble
Raspberry is bluetooth master, gathers data from beacons. 


Packages:
bluez-tools  #bluetooth utilities
bluez-utils
wpa-supplicant #for wifi
crng-tools  #quicker startup
wiringpi
openssh

Better yet, just $ cat /config | grep -i =y
