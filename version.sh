#!/bin/bash

echo -n '#define GIT_VERSION "' > version.h
git describe | tr -d '\n' >> version.h
echo '"' >> version.h