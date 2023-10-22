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

echo "#--------------------------------------------------#"

#aws s3 sync s3://${BUCKET}/common service/bin

echo "#--------------------------------------------------#"

git submodule update --init --remote --recursive .esp-aws-iot/

echo "#--------------------------------------------------#"

python $IDF_PATH/tools/idf_tools.py install xtensa-esp32-elf@esp-2019r2-8.2.0  &> /dev/null

echo "#--------------------------------------------------#"

. $IDF_PATH/export.sh &> /dev/null

echo "#--------------------------------------------------#"
echo "INFO: set-up done."
echo "#--------------------------------------------------#"
