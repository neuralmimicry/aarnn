#!/bin/bash

# Function to save images from Podman to tar files
save_images() {
    echo "Saving images from Podman..."
    images=("aarnn_vault" "aarnn_postgres" "aarnn" "visualiser")
    for image in "${images[@]}"; do
        podman save -o "${image}.tar" "${image}"
    done
}

# Function to load images into MicroK8s
load_images() {
    echo "Loading images into MicroK8s..."
    images=("aarnn_vault" "aarnn_postgres" "aarnn" "visualiser")
    for image in "${images[@]}"; do
        sudo microk8s ctr image import "${image}.tar"
    done
    confirm_images_loaded
}

# Function to confirm images are loaded into MicroK8s
confirm_images_loaded() {
    echo "Confirming images are loaded into MicroK8s..."
    images=("aarnn_vault" "aarnn_postgres" "aarnn" "visualiser")
    for image in "${images[@]}"; do
        if sudo microk8s ctr images list | grep -q "${image}"; then
            echo "Image ${image} is present"
        else
            echo "Image ${image} is missing"
            exit 1
        fi
    done
}

# Function to tag and push images to local registry
push_images_to_local_registry() {
    echo "Pushing images to local registry..."
    images=("aarnn_vault" "aarnn_postgres" "aarnn" "visualiser")
    registry="localhost:32000"

    for image in "${images[@]}"; do
        podman tag "${image}" "${registry}/${image}"
        retry_command 5 podman push "${registry}/${image}"
    done
}

# Function to retry a command up to a specified number of attempts
retry_command() {
    local retries=$1
    shift
    local command="$@"

    for ((i=1; i<=retries; i++)); do
        if $command; then
            return 0
        fi
        echo "Attempt $i failed. Retrying in 5 seconds..."
        sleep 5
    done

    echo "Command failed after $retries attempts."
    return 1
}

# Main script logic
case "$1" in
    save)
        save_images
        ;;
    load)
        load_images
        ;;
    push)
        push_images_to_local_registry
        ;;
    *)
        echo "Usage: $0 {save|load|push}"
        echo "  save - Save images from Podman to tar files"
        echo "  load - Load images into MicroK8s from tar files"
        echo "  push - Tag and push images to local registry"
        ;;
esac
