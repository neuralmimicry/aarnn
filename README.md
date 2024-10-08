![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/neuralmimicry/aarnn/cmake.yml?branch=master)
[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=neuralmimicry&url=https://github.com/neuralmimicry/aarnn&title=AARNN&language=&tags=github&category=software) 
# aarnn

The beginning of a new beginning of a...

Autonomic Asynchronous Recursive Neuromorphic Network (A.A.R.N.N)

AARNN (pronounced like the boy's name Aaron) is the infrastructure to support emergent cognitive behaviour in the form of tiered multi-agent distributed artificial intelligence created by Paul Isaac's under the self-funded project NeuralMimicry.

## Prerequisites
docker-desktop : Linux : https://docs.docker.com/desktop/install/linux-install/
```bash
sudo apt-get install libgtest-dev
sudo apt-get install libgmock-dev
```

## Installation

```bash
git clone git@github.com:neuralmimicry/aarnn.git
cd aarnn
mkdir build
cd build
cmake ..
make -j $(nproc)

ln -s configur/db_configuration.conf build/db_configuration.conf
copy simulation.conf -> build/simulation.com

```

## Running Postgres Container
*** Caution docker-station can use a lot of memory to run ***
Then running the image for the first time, the Host port (Ports) of the container needs
to be set, os it will forward it correctly e.g. 5432:5432
The password needs to be added in the Environments variables e.g. Variable -> POSTGRES_PASSWORD,
Value -> 'current password'

## Usage
Two parts, first creates the neurons and stores in a postgres database.
Second part visualises the neurons created in the first part.

```bash
./AARNN

./Visualiser
```

## Contributing
Feel free to contribute. We learn by doing.

## Sponsorship

## Docker ##
### Preparation
`visualise` is a GUI program. In order to run it from within a docker container,
docker containers must be able to share the windows manager with the host.
```bash
xhost +local:docker
```
Also running GUIs as root (as docker does) is by default disabled. The following
workaround lets visualiser run.
```bash
xhost si:localuser:root
```
After the run, the default should be re-applied:
```bash
xhost -si:localuser:root
```
### Create a common network between docker containers
```bash
docker network create -d bridge aarnn_network
```
## Build Images and Start Containers
```bash
SRC_DIR=$(realpath .) docker compose -f compose-build.yaml build
SRC_DIR=$(realpath .) docker compose -f compose-build.yaml up

SRC_DIR=$(realpath .) docker compose -f compose-run.yaml build
SRC_DIR=$(realpath .) docker compose -f compose-run.yaml up
```

## Individual build images
Password and Port are held in the .env file.

```bash
docker run -e POSTGRES_USER=postgres -e POSTGRES_PASSWORD=<INSET PASSWORD> -p <EXTERNAL PORT>:<INTERNAL PORT> -dit --name postgres --network aarnn_network postgres "docker-entrypoint.sh postgres"
```

## Create ARRAN image from prebuild AARNN
```bash
docker run -dit --name aarnn --network aarnn_network aarnn ./AARNN
```
