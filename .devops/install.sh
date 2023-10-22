#!/usr/bin/env bash

STAGE=
while [ $# -gt 0 ]; do
  case "$1" in
    -s|--stage)
      STAGE="${2}"; shift;;
    *)
      printf "***************************\n"
      printf "* Error: Invalid argument.*\n"
      printf "***************************\n"
      exit 1
  esac
  shift
done
STAGE=$(.devops/uuid.sh ${STAGE})
PREFIX="bin"
POSTFIX=
case "$STAGE" in
 product) POSTFIX="" ;;
 *) POSTFIX="-test" ;;
esac
SUBDOMAIN=${PREFIX}${POSTFIX}
BUCKET=${SUBDOMAIN}.lavigo.io

echo "#--------------------------------------------------#"
echo "INFO: starting set-up using..."
echo "stage: ${STAGE}"
echo "#--------------------------------------------------#"

export PATH="$HOME/.espressif/tools/xtensa-esp32-elf/esp-2019r2-8.2.0/xtensa-esp32-elf/bin:$PATH"
export IDF_PATH=$(pwd)/.esp-idf

cd .esp-idf
git fetch
git checkout release/v4.1
git submodule update --init --recursive
cd ..
cd .esp-aws-iot
git fetch
git checkout 7a5d204da1bdf7a3d3d8ee7f7287ce87acf29048
git submodule update --init --remote --recursive
cd ..
#git submodule update --init --remote --recursive .esp-aws-iot/

echo "#--------------------------------------------------#"

sudo apt-get install -y \
  git flex bison gperf python python-pip python-setuptools python-serial \
  python-click python-cryptography python-future python-pyparsing python-pyelftools \
  cmake ninja-build ccache libffi-dev libssl-dev jq awscli \
  &> /dev/null

echo "#--------------------------------------------------#"

python $IDF_PATH/tools/idf_tools.py install xtensa-esp32-elf@esp-2019r2-8.2.0  &> /dev/null

echo "#--------------------------------------------------#"

. $IDF_PATH/export.sh &> /dev/null

echo "#--------------------------------------------------#"

cp -f service/.aws-iot/src/aws_iot_mqtt_client_common_internal.c .esp-aws-iot/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_common_internal.c
cp -f service/.aws-iot/src/aws_iot_mqtt_client_yield.c .esp-aws-iot/aws-iot-device-sdk-embedded-C/src/aws_iot_mqtt_client_yield.c
cp -f service/.aws-iot/include/aws_iot_mqtt_client_common_internal.h .esp-aws-iot/aws-iot-device-sdk-embedded-C/include/aws_iot_mqtt_client_common_internal.h

aws s3 sync s3://${BUCKET}/common service/bin

echo "#--------------------------------------------------#"
echo "INFO: set-up done."
echo "#--------------------------------------------------#"

