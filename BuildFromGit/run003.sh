#!/bin/bash

# run003.sh - Build and upload multiple container images using Buildah

echo "Starting run003.sh: Building and uploading container images..."

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

# Define the image names, tags, containerfile paths, and build args
IMAGE_NAMES=("vault-image" "postgres-image" "compiler-image" "aarnn-image" "visualiser-image")
CONTAINERFILE_PATHS=(
    "./BuildFromGit/podman/Containerfile.compiler"
    "./BuildFromGit/podman/Containerfile.vault"
    "./BuildFromGit/podman/Containerfile.postgres"
    "./BuildFromGit/podman/Containerfile.aarnn"
    "./BuildFromGit/podman/Containerfile.visualiser"
)
BUILD_ARGS=(
    "--build-arg REPO_URL=https://github.com/neuralmimicry/aarnn.git"  # compiler-image
    ""  # vault-image has no build args
    ""  # postgres-image has no build args
    ""  # aarnn-image has no build args
    ""  # visualiser-image has no build args
)

IMAGE_TAG="latest"

# Function to push the image to a registry
push_image() {
    local local_image="$1"
    local registry_image="$2"
    echo "Tagging image $local_image for registry: $registry_image"
    buildah tag "$local_image" "$registry_image"

    echo "Pushing image $registry_image to registry..."
    buildah push "$registry_image"

    if [ $? -ne 0 ]; then
        echo "Failed to push the image $registry_image."
        exit 1
    fi

    echo "Image pushed successfully to $registry_image"
}

# Determine actions based on CLOUD_PROVIDER
case "$CLOUD_PROVIDER" in
    local)
        echo "CLOUD_PROVIDER is 'local'. No need to push images to a remote registry."
        PUSH_IMAGES=false
        ;;
    openstack)
        echo "CLOUD_PROVIDER is 'openstack'. Preparing to push images to OpenStack registry..."

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

        PUSH_IMAGES=true
        ;;
    aws)
        echo "CLOUD_PROVIDER is 'aws'. Preparing to push images to Amazon ECR..."

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

        PUSH_IMAGES=true
        ;;
    gcp)
        echo "CLOUD_PROVIDER is 'gcp'. Preparing to push images to Google Container Registry..."

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

        PUSH_IMAGES=true
        ;;
    azure)
        echo "CLOUD_PROVIDER is 'azure'. Preparing to push images to Azure Container Registry..."

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

        PUSH_IMAGES=true
        ;;
    *)
        echo "Unknown CLOUD_PROVIDER: $CLOUD_PROVIDER"
        exit 1
        ;;
esac

# Build and push images in order
NUM_IMAGES=${#IMAGE_NAMES[@]}

for (( i=0; i<$NUM_IMAGES; i++ )); do
    IMAGE_NAME="${IMAGE_NAMES[$i]}"
    CONTAINERFILE_PATH="${CONTAINERFILE_PATHS[$i]}"
    BUILD_ARG="${BUILD_ARGS[$i]}"
    LOCAL_IMAGE="${IMAGE_NAME}:${IMAGE_TAG}"

    # Check if the Containerfile exists
    if [ ! -f "$CONTAINERFILE_PATH" ]; then
        echo "Containerfile $CONTAINERFILE_PATH not found!"
        exit 1
    fi

    # Build the image using Buildah
    echo "Building image $LOCAL_IMAGE using Buildah..."
    buildah bud -f "$CONTAINERFILE_PATH" -t "$LOCAL_IMAGE" $BUILD_ARG

    if [ $? -ne 0 ]; then
        echo "Failed to build the image $LOCAL_IMAGE."
        exit 1
    fi

    echo "Image built successfully: $LOCAL_IMAGE"

    # Push the image if required
    if [ "$PUSH_IMAGES" = true ]; then
        case "$CLOUD_PROVIDER" in
            openstack)
                REGISTRY_IMAGE="${OS_REGISTRY_URL}/${OS_PROJECT_NAME}/${IMAGE_NAME}:${IMAGE_TAG}"
                ;;
            aws)
                # Create ECR repository if it doesn't exist
                aws ecr describe-repositories --repository-names "$IMAGE_NAME" --region "$AWS_REGION" >/dev/null 2>&1
                if [ $? -ne 0 ]; then
                    echo "Creating ECR repository: $IMAGE_NAME"
                    aws ecr create-repository --repository-name "$IMAGE_NAME" --region "$AWS_REGION"
                fi
                REGISTRY_IMAGE="${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com/${IMAGE_NAME}:${IMAGE_TAG}"
                ;;
            gcp)
                REGISTRY_IMAGE="gcr.io/${GCP_PROJECT_ID}/${IMAGE_NAME}:${IMAGE_TAG}"
                ;;
            azure)
                REGISTRY_IMAGE="${ACR_NAME}.azurecr.io/${IMAGE_NAME}:${IMAGE_TAG}"
                ;;
        esac

        push_image "$LOCAL_IMAGE" "$REGISTRY_IMAGE"
    fi

done

echo "run003.sh completed successfully."