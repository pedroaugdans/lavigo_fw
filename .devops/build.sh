#!/usr/bin/env bash

echo "#--------------------------------------------------#"
echo "INFO: starting set-up using..."
echo "#--------------------------------------------------#"

export PATH="$HOME/.espressif/tools/xtensa-esp32-elf/esp-2019r2-8.2.0/xtensa-esp32-elf/bin:$PATH"
export IDF_PATH=$(pwd)/.esp-idf

python $IDF_PATH/tools/idf_tools.py install xtensa-esp32-elf@esp-2019r2-8.2.0  &> /dev/null

echo "#--------------------------------------------------#"

. $IDF_PATH/export.sh &> /dev/null

echo "#--------------------------------------------------#"

cd service

ccache make

echo "#--------------------------------------------------#"

mkdir -p dist/

cp        version.txt    dist/version.txt
cp  build/hubware.bin    dist/hubware.bin
#cp bin/bootloader.bin dist/bootloader.bin
#cp   bin/ota_data.bin   dist/ota_data.bin
#cp bin/partitions.bin dist/partitions.bin

cd ..

echo "#--------------------------------------------------#"
echo "INFO: set-up done."
echo "#--------------------------------------------------#"
