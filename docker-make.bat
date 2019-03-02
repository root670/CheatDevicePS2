@echo off
set DOCKER_IMAGE=root670/ps2dev-gskit
docker run -t --rm -v "%CD%:/src" "%DOCKER_IMAGE%" make %*