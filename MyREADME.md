## Building the Docker Postgres DB

When you build the Docker image using the docker build command, you can pass in values for these environment variables using the --build-arg flag.

so for the aarnn db:

```
POSTGRES_USER=postgres
POSTGRES_PASSWORD=GeneTics99!
```

We pass these values into the docker build command:

```bash
docker build --build-arg POSTGRES_USER=postgres --build-arg POSTGRES_PASSWORD=GeneTics99! -t postgres-aarnn -f Dockerfile.postgres .

```

and then for docker run we use:
```bash
docker run --name postgres-aarnn -p 5432:5432 -e POSTGRES_USER=postgres -e POSTGRES_PASSWORD=GeneTics99! -e POSTGRES_DB=neurons -d postgres-aarnn
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



