#!/bin/bash

docker build -f Dockerfile.build -t jonnrb/hostapd_grpc_build .

if [ -z "${DOCKER_CONFIG}" ]; then
  DOCKER_CONFIG="${HOME}/.docker"
fi

docker run -it --rm \
  -v bazel_output:/output:rw \
  -e "DOCKER_CONFIG_CONTENTS=$(cat "${DOCKER_CONFIG}/config.json" |jq -c .)" \
  jonnrb/hostapd_grpc_build run //server:docker_push
