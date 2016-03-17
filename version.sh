#!/bin/bash

echo -n '#define GIT_VERSION "'
git describe | tr -d '\n'
echo '"'