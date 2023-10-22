#!/bin/sh
echo "Downloading Bootloader"

sudo chmod 666 /dev/ttyUSB0

python .esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x1000 ./service/bin/bootloader.bin 0x9000 ./service/bin/ota_data.bin 0x10000 ./service/dist/hubware.bin 0x8000 ./service/bin/partitions.bin
