#!/usr/bin/env bash

# Purpose: This script is intended to build Docker images for PostgreSQL,
# Aarnn, and Visualiser using Podman. It updates the registries.conf file
# to include Docker Hub, extracts environment variables from `.env` file.

# Exit immediately if a pipeline (which may consist of a single simple command),
# a list, or a compound command (see SHELL GRAMMAR above), exits with a non-zero status
set -e

# Check if the script is run as root or with sudo
# if [ "$EUID" -ne 0 ]; then
#     echo "Please run as root or with sudo"
#     exit 1
# fi

remove_if_exists() {
  if podman image exists "$1"; then
    echo "Removing existing image $1"
    podman rmi -f "$1"
  fi
  if podman container exists $1; then
    echo "Removing existing container $1"
    podman rm -f "$1"
  fi
}

# usage of 'sudo' may not always be necessary or desirable, consider utilizing
# a user with proper permissions to avoid using 'sudo'
echo "Adding Docker Hub to unqualified registries..."
{
echo ""
echo "unqualified-search-registries = [\"docker.io\"]"
echo ""
} | sudo tee -a /etc/containers/registries.conf > /dev/null

# Source the .env file.
# NOTE: Please ensure .env file does not contain any 'export' statements
echo "Loading environment variables..."
set -a
. ./.env
set +a

# Check required environment variables
required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DB" "POSTGRES_PORT_EXPOSE")
for var in "${required_vars[@]}"; do
  if [ -z "${!var}" ]; then
    echo "Error: Environment variable $var is not set."
    exit 1
  fi
done

# Build the Vault image
remove_if_exists vault
echo "Building Vault image..."
podman build -t vault -f Containerfile.vault .

# Build the PostgreSQL image
remove_if_exists postgres
echo "Building PostgreSQL image..."
podman build \
  --build-arg POSTGRES_USERNAME="${POSTGRES_USERNAME}" \
  --build-arg POSTGRES_PASSWORD="${POSTGRES_PASSWORD}" \
  --build-arg POSTGRES_DB="${POSTGRES_DB}" \
  --build-arg POSTGRES_PORT="${POSTGRES_PORT}" \
  --build-arg POSTGRES_PORT_EXPOSE="${POSTGRES_PORT_EXPOSE}" \
  -t postgres -f Containerfile.postgres .

# Build the Aarnn image
remove_if_exists aarnn
echo "Building AARNN image..."
podman build -t aarnn -f Containerfile.aarnnFullBuild .

# Build the Visualiser image
remove_if_exists visualiser
echo "Building Visualiser image..."
podman build -t visualiser -f Containerfile.visualiserFullBuild .
