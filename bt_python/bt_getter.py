from bluepy import btle
from time import sleep
import logging
import sys
import struct 

logging.basicConfig(level = logging.DEBUG)

logger = logging.getLogger(__name__)

def convert_to_le(el_number):
    """
    Pack as big endian, unpack as little endian
    Convert to 32bit number (discard not relevant 0s)
    return as XX.YY (readable temperature)
    """
    le_number = int(el_number, 16)
    le_number = struct.unpack("<I", struct.pack(">I", le_number))[0]
    le_number = le_number>>16 
    return le_number/100


if __name__ == "__main__":
    

    for attempt in range(10):
        try:
            logger.debug("Connecting...")
            dev = btle.Peripheral("DB:1A:7C:4D:6D:4B", addrType="random")
        except Exception as e:
            logger.error("Connection failed due to: {}, reconnecting...".format(e))
            sleep(2)
        else:
            break
    else:
        logger.error("Connection failed after all attempts. Examine remote sensor.")
        sys.exit()
    

    logger.debug("Services...")
    for svc in dev.services:
        logger.debug(str(svc))
        svc.getCharacteristics()


    tempService = dev.getServiceByUUID("0000181a-0000-1000-8000-00805f9b34fb")
    temp_char = tempService.getCharacteristics()[0]

    raw_temp = temp_char.read()
    logger.debug("Raw temperature: {}".format(raw_temp.hex()))

    readable_temp = convert_to_le(raw_temp.hex())
    logger.debug("Human-readable temperature: {}".format(readable_temp))