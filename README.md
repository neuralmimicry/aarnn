![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/neuralmimicry/aarnn/cmake.yml?branch=master)
[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=neuralmimicry&url=https://github.com/neuralmimicry/aarnn&title=AARNN&language=&tags=github&category=software) 
# aarnn

The beginning of a new beginning of a...

Autonomic Asynchronous Recursive Neuromorphic Network (A.A.R.N.N)

AARNN (pronounced like the boy's name Aaron) is the infrastructure to support emergent cognitive behaviour in the form of tiered multi-agent distributed artificial intelligence created by Paul Isaac's under the self-funded project NeuralMimicry.

## Preparation
The following sets up the environment for the AARNN project.
- ensures the required packages are installed to build the project
- sets up the required environment variables for the project
- runs the build process for the project
- runs the project
  - starts vault
  - starts the database
  - starts the AARNN component
  - runs the visualiser
  - runs the tests
  - runs the documentation
```bash
./run.sh
```

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

## Visualises
## Running Postgres Container
*** Caution docker-station can use a lot of memory to run ***
Then running the image for the first time, the Host port (Ports) of the container needs to be set, os it will forward it correctly e.g. 5432:5432
The password needs to be added in the Environments variables e.g. Variable -> POSTGRES_PASSWORD, Value -> 'current password'

## Usage
Two parts, first creates the neurons and stores in a postgres database.
Second part visualisers the neurons created in the first part.

```bash
./AARNN

./Visualiser
```

## Building the Docker Postgres DB

When you build the Docker image using the docker build command, you can pass in values for these environment variables using the --build-arg flag.

so for the aarnn db:

```
POSTGRES_USER=postgres
POSTGRES_PASSWORD=password
```

We pass these values into the docker build command:

```bash
docker build --build-arg POSTGRES_USER=postgres --build-arg POSTGRES_PASSWORD=password -t postgres-aarnn -f Dockerfile.postgres .

```

and then for docker run we use:
```bash
docker run --name postgres-aarnn -p 5432:5432 -e POSTGRES_USER=postgres -e POSTGRES_PASSWORD=password -e POSTGRES_DB=neurons -d postgres-aarnn
```

## Audio Sources

When the aarnn program is running we get the prompt:

```bash
1: alsa_input.pci-0000_00_1f.3-platform-skl_hda_dsp_generic.HiFi__hw_sofhdadsp__source
2: alsa_input.pci-0000_00_1f.3-platform-skl_hda_dsp_generic.HiFi__hw_sofhdadsp_6__source
3: alsa_output.pci-0000_01_00.1.hdmi-stereo.monitor
Select a source:
```

Here’s what each option means:


- `skl_hda_dsp_generic.HiFi__hw_sofhdadsp__sink.monitor`: This entry in the list refers to a monitor source for the audio output device on your system. The monitor source allows you to listen to the audio output from your system in real-time.

- `alsa_input.pci-0000_00_1f.3-platform-skl_hda_dsp_generic.HiFi__hw_sofhdadsp__source`: This entry in the list refers to an input source for audio recording. You can use this source to record audio from an external device such as a microphone or line-in input.

- `alsa_output.pci-0000_01_00.1.hdmi-stereo.monitor`: This entry in the list refers to a monitor source for the HDMI audio output device on your system. The monitor source allows you to listen to the audio output from your system in real-time.



## Licence

## Contributing
Feel free to contribute. We learn by doing.

## Sponsorship


##Initial steps
##Start PulseAudio (To use microphone/speakers for audio stimulation/activation)
./setup_pulse_audio.sh


##Podman##
## Create a common network between docker containers
podman network create my_net

## Build Image and Start Containers




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
