from bluepy import btle
from time import sleep
import logging
import sys
import struct 
import sqlite3

logging.basicConfig(level = logging.DEBUG)

logger = logging.getLogger(__name__)

def convert_to_le(el_number):
    """
    Pack as big endian, unpack as little endian
    Convert to 32bit number (discard not relevant 0s)
    return as XXYY (readable temperature)
    """
    le_number = int(el_number, 16)
    le_number = struct.unpack("<I", struct.pack(">I", le_number))[0]
    le_number = le_number>>16 
    return le_number

def log_temperature(temp):
    """ 
    Write sensor readout to the database.
    """
    temp = temp
    conn=sqlite3.connect('/var/www/ble_templog.db')
    curs=conn.cursor()
    curs.execute("INSERT INTO temps values(datetime('now', 'localtime'), (?))", (temp,))
    conn.commit()
    conn.close()

def read_chrc_value(dev, handle):
    assert(isinstance(handle, int))

    raw_val = dev.readCharacteristic(handle)
    logger.debug("Raw characteristic value: {}".format(raw_val.hex()))

    readable_val = convert_to_le(raw_val.hex())
    return readable_val


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

    raw_val = read_chrc_value(dev, 0x17)
    logger.debug("Temperature value: {} C".format(raw_val/100))
    raw_val = read_chrc_value(dev, 0x1e)
    logger.debug("Humidity value: {} %".format(raw_val/100))
    raw_val = read_chrc_value(dev, 0x22)
    logger.debug("Pressure value: {} hPa".format(raw_val))


    # logger.debug("Commiting sensor values ({}) to database...".format(readable_val/100))
    # log_temperature(readable_val)
    # logger.debug("Done!")