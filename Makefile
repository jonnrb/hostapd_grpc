# makefile wrapper to do bazel builds in a docker container

DOCKER_CONFIG ?= $(HOME)/.docker
DOCKER_CONFIG_CONTENTS := $(shell cat "$(DOCKER_CONFIG)/config.json" |jq -c .)

build: build_image
	docker run -it --rm -v bazel_output:/output:rw jonnrb/bazel_hostapd_grpc build //...

.PHONY: build build_image

build_image:
	docker build -f Dockerfile.build -t jonnrb/hostapd_grpc_build .

push: build_image
	docker run -it --rm \
	  -v bazel_output:/output:rw \
	  -e 'DOCKER_CONFIG_CONTENTS=$(DOCKER_CONFIG_CONTENTS)' \
	  jonnrb/hostapd_grpc_build run //server:docker_push
