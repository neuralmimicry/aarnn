![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/neuralmimicry/aarnn/cmake.yml?branch=master)
[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=neuralmimicry&url=https://github.com/neuralmimicry/aarnn&title=AARNN&language=&tags=github&category=software) 
# aarnn

The beginning of a new beginning of a...

Autonomic Asynchronous Recursive Neuromorphic Network (A.A.R.N.N)

AARNN (pronounced like the boy's name Aaron) is the infrastructure to support emergent cognitive behaviour in the form of tiered multi-agent distributed artificial intelligence created by Paul Isaac's under the self-funded project NeuralMimicry.

## Pre
docker-desktop : Linux : https://docs.docker.com/desktop/install/linux-install/

## Installation

```bash
git clone git@github.com:neuralmimicry/aarnn.git
cd aarnn
mkdir build
cd build
cmake ..
make

copy db_configuration.conf -> build/db_configuration.conf
copy simulation.conf -> build/simulation.com

```

## Running Postgres Container
*** Caution docker-station can user alot of memory to run ***
Then running the image for the first time, the Host port (Ports) of the container needs to be set, os it will forward it correctly e.g. 5432:5432
The password needs to be added in the Environments variables e.g. Variable -> POSTGRES_PASSWORD, Value -> 'current password'

## Usage
Two parts, first creates the neurons and stores in a postgres database.
Second part visualisers the neurons created in the first part.

```bash
./AARNN

./Visualiser
```

## Contributing
Feel free to contribute. We learn by doing.

## Sponsorship

## Docker ##
## Create a common network between docker containers
docker network create -d bridge my_net

## Build Image and Start Containers
docker compose up

## Individual build images
Password and Port are held in the .env file.

docker run -e POSTGRES_USER=postgres -e POSTGRES_PASSWORD=<INSET PASSWORD> -p <EXTERNAL PORT>:<INTERNAL PORT> -dit --name postgres --network my_net postgres "docker-entrypoint.sh postgres"

## Create ARRAN image from prebuild AARNN
docker run -dit --name aarnn --network my_net aarnn ./AARNN
