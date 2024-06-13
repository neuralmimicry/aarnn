#!/bin/bash

# Purpose: Manage containers for PostgreSQL, Aarnn, and Visualiser services using Podman

# Load environment variables
set -a
. ./.env
set +a

# Function to start containers
start_containers() {
    # Create the pod if it does not exist
    podman pod exists aarnn_pod || podman pod create --name aarnn_pod -p "${POSTGRES_PORT_EXPOSE}:${POSTGRES_PORT_EXPOSE}"

    # Source PulseAudio environment setup script
    . ./setup_pulse_audio.sh

    echo "Starting PostgreSQL container..."
    podman run --pod aarnn_pod --name postgres_aarnn -d \
               --health-cmd "pg_isready -U ${POSTGRES_USERNAME}" \
               --health-interval 5s \
               --health-timeout 5s \
               --health-retries 5 \
               --env-file ./.env \
               -e POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
               -e POSTGRES_HOST='localhost' \
               -e POSTGRES_PORT="${POSTGRES_PORT}" \
               postgres_aarnn

    echo "Waiting for PostgreSQL to start..."
    sleep 30

    # Start the AARNN and Visualiser containers
    echo "Starting AARNN container..."
    podman run --pod aarnn_pod --name aarnn -d \
        --env DISPLAY=":1" \
        --volume /tmp/.X11-unix:/tmp/.X11-unix \
        --env POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
        --env POSTGRES_DBNAME="${POSTGRES_DBNAME}" \
        --env POSTGRES_USERNAME="${POSTGRES_USERNAME}" \
        --env POSTGRES_HOST='localhost' \
        --env POSTGRES_PORT="${POSTGRES_PORT}" \
        --env PULSE_SERVER=unix:"${XDG_RUNTIME_DIR}"/pulse/native \
        --env PULSE_COOKIE=/root/.config/pulse/cookie \
        --env PULSE_SINK="${PULSE_SINK}" \
        --env PULSE_SOURCE="${PULSE_SOURCE}" \
        --volume "${XDG_RUNTIME_DIR}"/pulse/native:"${XDG_RUNTIME_DIR}"/pulse/native \
        --volume /usr/local/etc/pulse:/root/.config/pulse \
        aarnn

    echo "Starting Visualiser container with X11 forwarding..."
    xhost +local:
    podman run --pod aarnn_pod -d --name visualiser \
        --env DISPLAY=":2" \
        --volume /tmp/.X11-unix:/tmp/.X11-unix \
        --env POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
        --env POSTGRES_DBNAME="${POSTGRES_DBNAME}" \
        --env POSTGRES_USERNAME="${POSTGRES_USERNAME}" \
        --env POSTGRES_HOST='localhost' \
        --env POSTGRES_PORT="${POSTGRES_PORT}" \
        visualiser
}

# Function to stop containers
stop_containers() {
    echo "Stopping all containers..."
    podman pod stop aarnn_pod
    podman pod rm aarnn_pod
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
