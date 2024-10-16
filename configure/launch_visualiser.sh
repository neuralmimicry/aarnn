#!/bin/bash

source /app/.env

# Function to check required environment variables
check_variables() {
    required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DATABASE" "POSTGRES_HOST" "POSTGRES_PORT" "VAULT_ADDR" "VAULT_TOKEN")
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

# Check if the environment variables are set
check_variables

echo "Starting Visualiser..."
echo "VAULT_ADDR: $VAULT_ADDR"
echo "VAULT_TOKEN: $VAULT_TOKEN"

# Start gdb with the program and capture the backtrace on crash
#gdb -ex "set pagination off" -ex "run" -ex "bt" -ex "quit" /app/Visualiser > /app/gdb_backtrace.txt 2>&1
/app/Visualiser
# Print the backtrace to stdout
#cat /app/gdb_backtrace.txt
