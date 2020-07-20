from bluepy import btle
from time import sleep
import struct


if __name__ == "__main__":
    print ("Connecting...")
    dev = btle.Peripheral("DB:1A:7C:4D:6D:4B", addrType="random")

    raw = dev.readCharacteristic(0x17)
    print(raw.hex())
    raw = dev.readCharacteristic(0x17)
    print(raw)

    print ("Services...")
    for svc in dev.services:
        print (str(svc))
        svc.getCharacteristics()


    tempService = dev.getServiceByUUID("0000181a-0000-1000-8000-00805f9b34fb")
    temp_char = tempService.getCharacteristics()

    for char in temp_char:
        print("Characteristic handle: {}".format(hex(char.valHandle)))
        print("Characteristic value: {}".format(char.read()))