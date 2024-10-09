#!/bin/bash

# run003.sh - Build and upload the Containerfile.compiler image using Buildah

echo "Starting run003.sh: Building and uploading container image..."

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

# Check if CLOUD_PROVIDER variable is set
if [ -z "$CLOUD_PROVIDER" ]; then
    echo "CLOUD_PROVIDER is not set in $ENV_FILE"
    exit 1
fi

# Define the image name and tag
IMAGE_NAME="compiler-image"
IMAGE_TAG="latest"
LOCAL_IMAGE="${IMAGE_NAME}:${IMAGE_TAG}"

# Define the path to the Containerfile
CONTAINERFILE_PATH="./BuildFromGit/podman/Containerfile.compiler"

# For SSH - REPO_URL="git@github.com:neuralmimicry/aarnn.git"
REPO_URL="https://github.com/neuralmimicry/aarnn.git"

# Check if the Containerfile exists
if [ ! -f "$CONTAINERFILE_PATH" ]; then
    echo "Containerfile $CONTAINERFILE_PATH not found!"
    exit 1
fi

# Build the image using Buildah
echo "Building the image using Buildah..."
buildah bud -f "$CONTAINERFILE_PATH" -t "$LOCAL_IMAGE" --build-arg REPO_URL="$REPO_URL"

if [ $? -ne 0 ]; then
    echo "Failed to build the image."
    exit 1
fi

echo "Image built successfully: $LOCAL_IMAGE"

# Function to push the image to a registry
push_image() {
    local registry_image="$1"
    echo "Tagging image for registry: $registry_image"
    buildah tag "$LOCAL_IMAGE" "$registry_image"

    echo "Pushing image to registry: $registry_image"
    buildah push "$registry_image"

    if [ $? -ne 0 ]; then
        echo "Failed to push the image to $registry_image."
        exit 1
    fi

    echo "Image pushed successfully to $registry_image"
}

# Determine actions based on CLOUD_PROVIDER
case "$CLOUD_PROVIDER" in
    local)
        echo "CLOUD_PROVIDER is 'local'. No need to push the image to a remote registry."
        ;;
    openstack)
        echo "CLOUD_PROVIDER is 'openstack'. Pushing image to OpenStack registry..."

        # Check for required OpenStack variables
        if [ -z "$OS_USERNAME" ] || [ -z "$OS_PASSWORD" ] || [ -z "$OS_AUTH_URL" ] || [ -z "$OS_PROJECT_NAME" ]; then
            echo "OpenStack credentials (OS_USERNAME, OS_PASSWORD, OS_AUTH_URL, OS_PROJECT_NAME) are not set in $ENV_FILE"
            exit 1
        fi

        # Log in to OpenStack registry (assuming it's compatible with Docker registry)
        echo "Logging in to OpenStack container registry..."
        OS_REGISTRY_URL="${OS_REGISTRY_URL:-registry.example.com}"  # Replace with actual registry URL

        echo "$OS_PASSWORD" | buildah login --username "$OS_USERNAME" --password-stdin "$OS_REGISTRY_URL"

        if [ $? -ne 0 ]; then
            echo "Failed to log in to OpenStack registry."
            exit 1
        fi

        # Push the image
        REGISTRY_IMAGE="${OS_REGISTRY_URL}/${OS_PROJECT_NAME}/${IMAGE_NAME}:${IMAGE_TAG}"
        push_image "$REGISTRY_IMAGE"
        ;;
    aws)
        echo "CLOUD_PROVIDER is 'aws'. Pushing image to Amazon ECR..."

        # Check for required AWS variables
        if [ -z "$AWS_ACCOUNT_ID" ] || [ -z "$AWS_REGION" ]; then
            echo "AWS_ACCOUNT_ID and AWS_REGION must be set in $ENV_FILE"
            exit 1
        fi

        # Get ECR login password and log in
        echo "Logging in to Amazon ECR..."
        aws ecr get-login-password --region "$AWS_REGION" | buildah login --username AWS --password-stdin "$AWS_ACCOUNT_ID.dkr.ecr.$AWS_REGION.amazonaws.com"

        if [ $? -ne 0 ]; then
            echo "Failed to log in to Amazon ECR."
            exit 1
        fi

        # Create ECR repository if it doesn't exist
        aws ecr describe-repositories --repository-names "$IMAGE_NAME" --region "$AWS_REGION" >/dev/null 2>&1
        if [ $? -ne 0 ]; then
            echo "Creating ECR repository: $IMAGE_NAME"
            aws ecr create-repository --repository-name "$IMAGE_NAME" --region "$AWS_REGION"
        fi

        # Push the image
        REGISTRY_IMAGE="${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com/${IMAGE_NAME}:${IMAGE_TAG}"
        push_image "$REGISTRY_IMAGE"
        ;;
    gcp)
        echo "CLOUD_PROVIDER is 'gcp'. Pushing image to Google Container Registry..."

        # Check for required GCP variables
        if [ -z "$GCP_PROJECT_ID" ]; then
            echo "GCP_PROJECT_ID must be set in $ENV_FILE"
            exit 1
        fi

        # Log in to GCR
        echo "Logging in to Google Container Registry..."
        gcloud auth configure-docker

        if [ $? -ne 0 ]; then
            echo "Failed to configure Docker authentication for GCR."
            exit 1
        fi

        # Push the image
        REGISTRY_IMAGE="gcr.io/${GCP_PROJECT_ID}/${IMAGE_NAME}:${IMAGE_TAG}"
        push_image "$REGISTRY_IMAGE"
        ;;
    azure)
        echo "CLOUD_PROVIDER is 'azure'. Pushing image to Azure Container Registry..."

        # Check for required Azure variables
        if [ -z "$ACR_NAME" ]; then
            echo "ACR_NAME must be set in $ENV_FILE"
            exit 1
        fi

        # Log in to ACR
        echo "Logging in to Azure Container Registry..."
        az acr login --name "$ACR_NAME"

        if [ $? -ne 0 ]; then
            echo "Failed to log in to Azure Container Registry."
            exit 1
        fi

        # Push the image
        REGISTRY_IMAGE="${ACR_NAME}.azurecr.io/${IMAGE_NAME}:${IMAGE_TAG}"
        push_image "$REGISTRY_IMAGE"
        ;;
    *)
        echo "Unknown CLOUD_PROVIDER: $CLOUD_PROVIDER"
        exit 1
        ;;
esac

echo "run003.sh completed successfully."
