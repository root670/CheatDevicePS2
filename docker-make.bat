@echo off
set DOCKER_IMAGE=root670/ps2dev-docker
docker run -t --rm -v "%CD%:/build" "%DOCKER_IMAGE%" make %*