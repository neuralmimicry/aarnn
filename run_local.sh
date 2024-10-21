#!/bin/bash

# run_local.sh - Run aarnn or visualiser locally while connecting to Vault and Postgres containers

# Usage:
#   ./run_local.sh aarnn      # To run the aarnn application
#   ./run_local.sh visualiser # To run the visualiser application

# Check if the application to run is specified
if [ -z "$1" ]; then
    echo "Usage: $0 [aarnn|visualiser]"
    exit 1
fi

PROJECT_DIR=$(dirname $(realpath $0))

APP_NAME="$1"

# Define the path to the .env file
ENV_FILE="./.env"

# Check if the .env file exists
if [ ! -f "$ENV_FILE" ]; then
    echo "Environment file $ENV_FILE not found!"
    exit 1
fi

# Source the .env file and export variables
set -o allexport
source "$ENV_FILE"
set +o allexport

# Function to wait for a container to be ready
wait_for_container() {
    local container_name="$1"
    local check_command="$2"
    local max_attempts=30
    local attempt=1

    echo "Waiting for $container_name to be ready..."
    while [ $attempt -le $max_attempts ]; do
        if eval "$check_command" >/dev/null 2>&1; then
            echo "$container_name is ready."
            return 0
        else
            echo "Waiting for $container_name to be ready... ($attempt/$max_attempts)"
            sleep 2
        fi
        attempt=$((attempt + 1))
    done

    echo "$container_name did not become ready in time."
    return 1
}

# Retrieve POSTGRES_ variables from the latest aarnn image
AARNN_IMAGE="aarnn-image:latest"
TEMP_CONTAINER_NAME="aarnn_temp_container"

echo "Creating temporary container from $AARNN_IMAGE to extract /app/.env..."
# Create a temporary container without starting it
podman create --name "$TEMP_CONTAINER_NAME" "$AARNN_IMAGE" >/dev/null

if [ $? -ne 0 ]; then
    echo "Failed to create temporary container from $AARNN_IMAGE."
    exit 1
fi

# Copy /app/.env from the temporary container
echo "Copying /app/.env from temporary container..."
podman cp "$TEMP_CONTAINER_NAME":/app/.env ./aarnn_container.env

if [ $? -ne 0 ]; then
    echo "Failed to copy /app/.env from temporary container."
    # Clean up the temporary container
    podman rm "$TEMP_CONTAINER_NAME" >/dev/null
    exit 1
fi

# Remove the temporary container
podman rm "$TEMP_CONTAINER_NAME" >/dev/null

# Define network name
NETWORK_NAME="aarnn_network"
export NETWORK_NAME

# Check if network exists, and create it if it doesn't
if ! podman network exists "$NETWORK_NAME"; then
    echo "Creating network $NETWORK_NAME"
    podman network create "$NETWORK_NAME"
fi

# Extract POSTGRES_ variables from aarnn_container.env
echo "Extracting POSTGRES_ variables from aarnn_container.env..."
if [ ! -f "./aarnn_container.env" ]; then
    echo "Failed to find aarnn_container.env."
    exit 1
fi

# Read the variables from the file
POSTGRES_VARS=$(grep '^POSTGRES_' ./aarnn_container.env)

# Export the variables
echo "Exporting POSTGRES_ variables..."
while IFS= read -r line; do
    # Ignore empty lines
    if [ -n "$line" ]; then
        # Export the variable
        export "$line"
        echo "$line"
    fi
done <<< "${POSTGRES_VARS}"

# Remove the temporary env file
rm -f ./aarnn_container.env

# Ensure Vault container is running and retrieve VAULT_TOKEN
VAULT_CONTAINER_NAME="vault"

# Set VAULT_ADDR to localhost since we're running the application locally
VAULT_ADDR="http://localhost:8200"
VAULT_API_ADDR="http://vault:8200"

# Set environment variables for the application
export VAULT_ADDR
export VAULT_API_ADDR
echo "VAULT_ADDR set to: ${VAULT_ADDR}"
echo "VAULT_API_ADDR set to: ${VAULT_API_ADDR}"

# Check if Vault container exists
if podman ps -a --format "{{.Names}}" | grep -q "^$VAULT_CONTAINER_NAME$"; then
    # Check if the container is running
    if ! podman ps --format "{{.Names}}" | grep -q "^$VAULT_CONTAINER_NAME$"; then
        echo "Starting existing Vault container..."
        podman start "${VAULT_CONTAINER_NAME}"
    else
        echo "Vault container already running."
    fi
else
    echo "Starting Vault container..."
#        --network slirp4netns:port_handler=slirp4netns \
    podman run -d --name "${VAULT_CONTAINER_NAME}" \
        --network "${NETWORK_NAME}" \
        -p 8200:8200 \
        -e VAULT_DEV_LISTEN_ADDRESS="0.0.0.0:8200" \
        -e VAULT_API_ADDR="${VAULT_API_ADDR}" \
        vault-image:latest
fi

# Wait for Vault to be ready
VAULT_CHECK_COMMAND="podman exec \"${VAULT_CONTAINER_NAME}\" vault status >/dev/null 2>&1"
if ! wait_for_container "Vault" "${VAULT_CHECK_COMMAND}"; then
    podman logs "$VAULT_CONTAINER_NAME"
    exit 1
fi

# Retrieve VAULT_TOKEN and set VAULT_ADDR to localhost
VAULT_TOKEN=$(podman exec "${VAULT_CONTAINER_NAME}" cat /home/vault/.vault-token)

if [ -z "$VAULT_TOKEN" ]; then
    echo "Failed to retrieve Vault token."
    podman logs "$VAULT_CONTAINER_NAME"
    exit 1
fi

export VAULT_TOKEN

echo "VAULT_TOKEN retrieved: ${VAULT_TOKEN}"

# Ensure Postgres container is running
POSTGRES_CONTAINER_NAME="postgres"
# POSTGRES_* variables are already exported from the aarnn_container.env

# Ensure POSTGRES_HOST and POSTGRES_PORT are set
export POSTGRES_HOST="${POSTGRES_HOST:-postgres}"
export POSTGRES_PORT="${POSTGRES_PORT:-5432}"

echo "POSTGRES_USERNAME: ${POSTGRES_USERNAME}"
echo "POSTGRES_PASSWORD: ${POSTGRES_PASSWORD}"
echo "POSTGRES_DB: ${POSTGRES_DB}"
echo "POSTGRES_HOST: ${POSTGRES_HOST}"
echo "POSTGRES_PORT: ${POSTGRES_PORT}"

if podman ps -a --format "{{.Names}}" | grep -q "^$POSTGRES_CONTAINER_NAME$"; then
    # Check if the container is running
    if ! podman ps --format "{{.Names}}" | grep -q "^$POSTGRES_CONTAINER_NAME$"; then
        echo "Starting existing Postgres container..."
        podman start "${POSTGRES_CONTAINER_NAME}"
    else
        echo "Postgres container already running."
    fi
else
    echo "Starting Postgres container..."
    podman run -d --name "${POSTGRES_CONTAINER_NAME}" \
        --network "${NETWORK_NAME}" \
        -e POSTGRES_USER="${POSTGRES_USERNAME}" \
        -e POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
        -e POSTGRES_DB="${POSTGRES_DB}" \
        -p "${POSTGRES_PORT}":"${POSTGRES_PORT_EXPOSE}" \
        postgres-image:latest
fi

# Wait for Postgres to be ready
POSTGRES_CHECK_COMMAND="podman exec \"$POSTGRES_CONTAINER_NAME\" pg_isready -U \"${POSTGRES_USERNAME}\" -h localhost"
if ! wait_for_container "Postgres" "$POSTGRES_CHECK_COMMAND"; then
    podman logs "$POSTGRES_CONTAINER_NAME"
    exit 1
fi

# Run the specified application
case "$APP_NAME" in
    aarnn)
        echo "Running AARNN application locally..."
        # Assuming the AARNN executable is located at ./build/AARNN
        if [ ! -f "./build/AARNN" ]; then
            echo "AARNN executable not found! Building AARNN..."
            # Build the AARNN application
            cd "$PROJECT_DIR" || exit 1
            rmdir -rf build || true
            mkdir -p build && cd build || exit 1
            cmake ..
            make
        fi
        # Run the aarnn application
        cd "$PROJECT_DIR" || exit 1
        cd build || exit 1
        # Start gdb with the program and capture the backtrace on crash
        # gdb -ex "set pagination off" -ex "run" -ex "bt" -ex "quit" ./AARNN > ./gdb_backtrace.txt 2>&1
        ./AARNN
        # Print the backtrace to stdout
        # cat ./gdb_backtrace.txt
        ;;
    visualiser)
        echo "Running Visualiser application locally..."
        # Assuming the Visualiser executable is located at ./build/Visualiser
        if [ ! -f "./build/Visualiser" ]; then
            echo "Visualiser executable not found! Building Visualiser..."
            # Build the Visualiser application
            cd "$PROJECT_DIR" || exit 1
            rmdir -rf build || true
            mkdir -p build && cd build || exit 1
            cmake ..
            make
        fi
        # Run the visualiser application
        cd "$PROJECT_DIR" || exit 1
        cd build || exit 1
        ./Visualiser
        ;;
    *)
        echo "Invalid application name: $APP_NAME"
        echo "Usage: $0 [aarnn|visualiser]"
        exit 1
        ;;
esac

echo "Application $APP_NAME exited."
cd "$PROJECT_DIR" || exit 1
