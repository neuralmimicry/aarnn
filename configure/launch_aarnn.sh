#!/bin/bash

echo "Configuring PulseAudio..."
# Check if the environment variables are set
if [ -z "$PULSE_SERVER" ]; then
  echo "PULSE_SERVER is not set. Exiting."
  exit 1
fi

if [ -z "$PULSE_SINK" ]; then
  echo "PULSE_SINK is not set. Exiting."
  exit 1
fi

if [ -z "$PULSE_SOURCE" ]; then
  echo "PULSE_SOURCE is not set. Exiting."
  exit 1
fi

# Set the PulseAudio sink and source
pactl set-default-sink "$PULSE_SINK"
pactl set-default-source "$PULSE_SOURCE"
pactl info

echo "Starting AARNN..."
/app/build/AARNN
