#!/bin/sh

set -u
set -e

# Add a console on tty1
if [ -e ${TARGET_DIR}/etc/inittab ]; then
    grep -qE '^tty1::' ${TARGET_DIR}/etc/inittab || \
	sed -i '/GENERIC_SERIAL/a\
tty1::respawn:/sbin/getty -L  tty1 0 vt100 # HDMI console' ${TARGET_DIR}/etc/inittab
fi

#############################################################################################################################################
## PermitRootLogin for ssh
sed -i '/#PermitRootLogin prohibit-password/c\PermitRootLogin yes' output/target/etc/ssh/sshd_config 


################# Enabling Wifi #######################################
if [ ! -f "${TARGET_DIR}/etc/wpa_supplicant.conf" ]; then
	sudo touch ${TARGET_DIR}/etc/wpa_supplicant.conf
fi
sudo chmod 777 ${TARGET_DIR}/etc/wpa_supplicant.conf

## make ip config
cat > output/target/etc/network/interfaces << EOL
# interface file auto-generated by buildroot

auto lo
iface lo inet loopback

auto wlan0
iface wlan0 inet dhcp
  pre-up wpa_supplicant -D nl80211 -i wlan0 -c /etc/wpa_supplicant.conf -B
  post-down killall -q wpa_supplicant


EOL

echo "Overwrite password and network name! Then delete this block" 1>&2
exit 64

cat > output/target/etc/wpa_supplicant.conf << EOL
ctrl_interface=/var/run/wpa_supplicant
ap_scan=1

network={
   ssid="Overwrite!"
   psk="Overwrite!"
}


EOL


#write enable_uart=1
if ! grep -qE '^enable_uart=1' "${BINARIES_DIR}/rpi-firmware/config.txt"; then
			cat << __EOF__ >> "${BINARIES_DIR}/rpi-firmware/config.txt"

# enable rpi3 ttyS0 serial console
enable_uart=1
__EOF__
fi

#write core_freq=250
if ! grep -qE '^core_freq=250' "${BINARIES_DIR}/rpi-firmware/config.txt"; then
			cat << __EOF__ >> "${BINARIES_DIR}/rpi-firmware/config.txt"

# adjust core_freq to enable BT adapter
core_freq=250
__EOF__
fi





