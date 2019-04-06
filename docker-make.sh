#!/bin/bash
DOCKER_IMAGE=root670/ps2dev-docker
exec docker run -t --rm -v "$PWD:/build" "$DOCKER_IMAGE" make "$@"
