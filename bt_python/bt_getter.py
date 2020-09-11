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

def log_data_to_db(table_name, value):
    """ 
    Write sensor readout to the database.
    """
    conn=sqlite3.connect('/var/www/ble_templog.db')
    sqlite_cmd = "INSERT INTO {table_name} values(datetime('now', 'localtime'), ({value}))".format(table_name=table_name, value=value)
    curs=conn.cursor()
    curs.execute(sqlite_cmd)
    conn.commit()
    conn.close()

def read_chrc_value(dev, handle):
    """
    Read characteristic using service UUID
    Convert to readable value and return
    """
    assert(isinstance(handle, int))

    raw_val = dev.readCharacteristic(handle)
    logger.debug("Raw characteristic value: {}".format(raw_val.hex()))

    readable_val = convert_to_le(raw_val.hex())
    return readable_val


if __name__ == "__main__":
    handle_to_char_map = {
        "temperature": 0x17,
        "humidity": 0x1e,
        "pressure": 0x22,
    }


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

    temp_val = read_chrc_value(dev, handle_to_char_map["temperature"])/100
    logger.debug("Temperature value: {} C".format(temp_val))
    hum_val = read_chrc_value(dev, handle_to_char_map["humidity"])/100
    logger.debug("Humidity value: {} %".format(hum_val))
    pressure_val = read_chrc_value(dev, handle_to_char_map["pressure"])
    logger.debug("Pressure value: {} hPa".format(pressure_val))

    table_to_value_map = {
        "temps": temp_val,
        "pressure": pressure_val,
        "humidity": hum_val
    }
    for table,value in table_to_value_map.items():
        logger.debug("Commiting sensor value ({}) to table...".format(table))
        log_data_to_db(table, value)
        logger.debug("Done!")