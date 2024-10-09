#!/bin/bash

#Set the environment variable based on command line arguments or any other logic
export GIT_USERNAME=$1
export BRANCH_RELEASE=$2
export GIT_PAT=$3


# Run Docker Compose build
#docker compose build
docker compose up -d