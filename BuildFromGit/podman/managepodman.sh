#!/bin/bash

# Purpose: Manage containers for PostgreSQL, Aarnn, and Visualiser services using Podman

# Load environment variables
set -a
. ./.env
set +a

# Function to check required environment variables
check_variables() {
    required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DB" "POSTGRES_PORT_EXPOSE" "PULSE_SINK" "PULSE_SOURCE" "XDG_RUNTIME_DIR" "POSTGRES_HOST" "POSTGRES_PORT")
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

# Function to get the gateway IP of the specified network
get_network_gateway() {
    local network_name=$1
    podman network inspect "$network_name" | jq -r '.[0].subnets[0].gateway'
}

# Function to start containers
start_containers() {
    # Check environment variables
    check_variables

    # Create the pod if it does not exist
    podman network exists aarnn_network || podman network create --driver bridge aarnn_network
    podman pod exists aarnn_pod || podman pod create --name aarnn_pod --network aarnn_network -p "${POSTGRES_PORT_EXPOSE}:${POSTGRES_PORT_EXPOSE}" -p 8200:8200

    # Get the gateway IP of the aarnn_network
    GATEWAY_IP=$(get_network_gateway "aarnn_network")
    if [[ -z "$GATEWAY_IP" ]]; then
        echo "Error: Could not determine the gateway IP of the aarnn_network."
        exit 1
    else
        echo "Gateway IP of aarnn_network: $GATEWAY_IP"
    fi

    # Source PulseAudio environment setup script
    . ./setup_pulse_audio.sh

    echo "Starting Vault container..."
    podman run --pod aarnn_pod --name vault -d \
               --network aarnn_network \
               --env-file ./.env \
               vault /bin/bash -c "bash /usr/local/bin/start_vault.sh"

    # Wait for the Vault container to initialize and extract the token
    echo "Waiting for Vault to start..."
    sleep 30
    podman exec vault cat /opt/vault/logs/vault_env.sh > vault_env.sh
    chmod +x vault_env.sh
    . ./vault_env.sh
    echo "$VAULT_ADDR"
    echo "$VAULT_API_ADDR"
    echo "$VAULT_TOKEN"
    export VAULT_API_ADDR
    export VAULT_ADDR
    export VAULT_TOKEN
    export XDG_RUNTIME_DIR

    echo "Starting PostgreSQL container..."
    podman run --pod aarnn_pod --name postgres -d \
               --network aarnn_network \
               --health-cmd "pg_isready -U ${POSTGRES_USERNAME}" \
               --health-interval 5s \
               --health-timeout 5s \
               --health-retries 5 \
               --env-file ./.env \
               --env POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
               --env POSTGRES_DB="${POSTGRES_DB}" \
               --env POSTGRES_HOST="${POSTGRES_HOST}" \
               --env POSTGRES_PORT="${POSTGRES_PORT}" \
               postgres

    echo "Waiting for PostgreSQL to start..."
    sleep 30

    # Start multiple AARNN containers with unique IDs
    echo "Starting AARNN containers..."
    for i in $(seq 1 5); do
        AARNN_CONTAINER_ID="aarnn_$i"
        podman run --pod aarnn_pod --name "$AARNN_CONTAINER_ID" -d \
            --network aarnn_network \
            --cap-add=NET_ADMIN \
            --cap-add=NET_RAW \
            --env DISPLAY=":1" \
            --volume /tmp/.X11-unix:/tmp/.X11-unix \
            --env VAULT_ADDR="${VAULT_ADDR}" \
            --env VAULT_API_ADDR="${VAULT_API_ADDR}" \
            --env VAULT_TOKEN="${VAULT_TOKEN}" \
            --env PULSE_SERVER=unix:"${XDG_RUNTIME_DIR}"/pulse/native \
            --env PULSE_COOKIE=/root/.config/pulse/cookie \
            --env PULSE_SINK="${PULSE_SINK}" \
            --env PULSE_SOURCE="${PULSE_SOURCE}" \
            --env AARNN_CONTAINER_ID="${AARNN_CONTAINER_ID}" \
            --volume "${XDG_RUNTIME_DIR}"/pulse/native:"${XDG_RUNTIME_DIR}"/pulse/native \
            --volume /usr/local/etc/pulse:/root/.config/pulse \
            aarnn
    done

    # Pause to allow AARNN to create initial data
    sleep 30

    echo "Starting Visualiser container with X11 forwarding..."
    xhost +local:
    podman run --pod aarnn_pod -d --name visualiser \
        --network aarnn_network \
        --cap-add=NET_ADMIN \
        --cap-add=NET_RAW \
        --env DISPLAY=":2" \
        --volume /tmp/.X11-unix:/tmp/.X11-unix \
        --env POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
        --env POSTGRES_DB="${POSTGRES_DB}" \
        --env POSTGRES_USERNAME="${POSTGRES_USERNAME}" \
        --env POSTGRES_HOST="${POSTGRES_HOST}" \
        --env POSTGRES_PORT="${POSTGRES_PORT}" \
        --env VAULT_ADDR="${VAULT_ADDR}" \
        --env VAULT_API_ADDR="${VAULT_API_ADDR}" \
        --env VAULT_TOKEN="${VAULT_TOKEN}" \
        visualiser
}

# Function to stop containers
stop_containers() {
    echo "Stopping all containers..."
    podman pod stop aarnn_pod
    podman pod rm aarnn_pod
    podman network rm aarnn_network
    xhost -local:
}

# Function to show logs
show_logs() {
    echo "Fetching logs from $1..."
    podman logs -f "$1"
}

# Main script logic based on passed argument
case "$1" in
    start)
        start_containers
        ;;
    stop)
        stop_containers
        ;;
    logs)
        if [[ "$2" == "" ]]; then
            echo "Please specify the container name for logs (aarnn or visualiser)"
        else
            show_logs "$2"
        fi
        ;;
    *)
        echo "Usage: $0 {start|stop|logs <container_name>}"
        echo "  start - Starts all containers"
        echo "  stop - Stops all containers and removes them"
        echo "  logs <container_name> - Tails logs from the specified container (aarnn or visualiser)"
        ;;
esac
