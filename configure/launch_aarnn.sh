#!/bin/bash

source /app/.env

# Function to check required environment variables
check_variables() {
    required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DB" "POSTGRES_HOST" "POSTGRES_PORT" "PULSE_SERVER" "PULSE_COOKIE" "PULSE_SINK" "PULSE_SOURCE" "VAULT_ADDR" "VAULT_API_ADDR" "VAULT_TOKEN")
    for var in "${required_vars[@]}"; do
      # Remove quotes from the variable value
      value=$(echo "${!var}" | sed 's/^"//; s/"$//')
      export "$var"="$value"

      if [ -z "$value" ]; then
        echo "Error: Environment variable $var is not set."
        exit 1
      fi
    done
}

echo "Configuring PulseAudio..."
# Check if the environment variables are set
check_variables

# Set the PulseAudio sink and source
pactl set-default-sink "$PULSE_SINK"
pactl set-default-source "$PULSE_SOURCE"
pactl info

echo "Starting AARNN..."
echo "VAULT_ADDR: $VAULT_ADDR"
echo "VAULT_API_ADDR: $VAULT_API_ADDR"
echo "VAULT_TOKEN: $VAULT_TOKEN"

# Start gdb with the program and capture the backtrace on crash
#gdb -ex "set pagination off" -ex "run" -ex "bt" -ex "quit" /app/AARNN > /app/gdb_backtrace.txt 2>&1
/app/AARNN
# Print the backtrace to stdout
#cat /app/gdb_backtrace.txt
