#!/usr/bin/env bash

cp ../bt_python/bt_getter.py /usr/bin/bt_getter.py
chmod +x /usr/bin/bt_getter.py

cp ble_logger.service /etc/systemd/system/ble_logger.service
chmod 644 /etc/systemd/system/ble_logger.service

cp ble_logger.timer /etc/systemd/system/ble_logger.timer
chmod 644 /etc/systemd/system/ble_logger.timer