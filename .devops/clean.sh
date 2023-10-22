echo "#--------------------------------------------------#"
echo "INFO: starting set-up using..."
echo "#--------------------------------------------------#"
export PATH="$HOME/.espressif/tools/xtensa-esp32-elf/esp-2019r2-8.2.0/xtensa-esp32-elf/bin:$PATH"
export IDF_PATH=$(pwd)/.esp-idf
install xtensa-esp32-elf@esp-2019r2-8.2.0  &> /dev/null
echo "#--------------------------------------------------#"
. $IDF_PATH/export.sh &> /dev/null
echo "#--------------------------------------------------#"
cd service
sudo chmod 666 /dev/ttyUSB0
ccache make clean
echo "#--------------------------------------------------#"
