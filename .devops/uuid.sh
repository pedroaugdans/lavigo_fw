#!/usr/bin/env bash

STAGE=${1}
BRANCH=$(git rev-parse --abbrev-ref HEAD)
UUID=

if [ "${STAGE}" != "" ]; then
  UUID=${STAGE}
elif [ "${BRANCH}" == "develop" ] || [ "${BRANCH}" == "release" ] || [ "${BRANCH}" == "master" ]  || [ "${BRANCH}" == "product" ]; then
  UUID=${BRANCH}
else
  UUID=$(echo ${BRANCH} | sha1sum | head -c 8)
fi

echo ${UUID}
