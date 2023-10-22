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
echo "INFO: starting deployment using..."
echo "stage: ${STAGE}"
echo "#--------------------------------------------------#"

cd service

aws s3 sync s3://${BUCKET}/${STAGE} dist

cd ..

echo "#--------------------------------------------------#"
echo "INFO: deployment done."
echo "#--------------------------------------------------#"
