#!/usr/bin/env bash

SERVICE=${1:-$(basename -s .git `git config --get remote.origin.url`)}

PIPELINES="bitbucket-pipelines.yml"
GITIGNORE=".gitignore"

SRC=$(cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd)
DST=$(pwd)

echo "#--------------------------------------------------#"
echo "INFO: starting initialization"

read -p "Are you sure you want to reset pipelines? [y/N] " -n 2 -r
if [[ $REPLY =~ ^[Yy]$ ]] ; then
  echo "copying" ${SRC}/${PIPELINES}
  echo "to     " ${DST}/${PIPELINES}
  echo ...
  cp ${SRC}/${PIPELINES} ${DST}/${PIPELINES}
  echo done!
fi

read -p "Are you sure you want to reset gitignore? [y/N] " -n 2 -r
if [[ $REPLY =~ ^[Yy]$ ]] ; then
  echo "copying" ${SRC}/${GITIGNORE}
  echo "to     " ${DST}/${GITIGNORE}
  echo ...
  cp ${SRC}/${GITIGNORE} ${DST}/${GITIGNORE}
  echo done!
fi

echo "#--------------------------------------------------#"
echo "INFO: initialization done."
echo "#--------------------------------------------------#"
