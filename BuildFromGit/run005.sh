#!/bin/bash

# run005.sh - Delete older (non-running) images from the container registry or local storage

echo "Starting run005.sh: Deleting older (non-running) images..."

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

# Define the image names
IMAGES=("vault-image" "postgres-image" "compiler-image" "aarnn-image" "visualiser-image")

# Define the image tag to keep (assuming 'latest')
IMAGE_TAG="latest"

case "$CLOUD_PROVIDER" in
    local)
        echo "CLOUD_PROVIDER is 'local'. Deleting older images from local storage..."
        for IMAGE_NAME in "${IMAGES[@]}"; do
            echo "Processing image: $IMAGE_NAME"
            # List all images with this name
            IMAGE_IDS=$(podman images --format "{{.ID}} {{.Repository}}:{{.Tag}}" | grep "^.* $IMAGE_NAME:" | awk '{print $1}')
            # Get the ID of the latest image
            LATEST_IMAGE_ID=$(podman images --format "{{.ID}} {{.Repository}}:{{.Tag}}" | grep "^.* $IMAGE_NAME:$IMAGE_TAG\$" | awk '{print $1}')
            for ID in $IMAGE_IDS; do
                if [ "$ID" != "$LATEST_IMAGE_ID" ]; then
                    echo "Deleting image ID $ID"
                    podman rmi "$ID"
                else
                    echo "Skipping latest image ID $ID"
                fi
            done
        done
        ;;
    openstack)
        echo "CLOUD_PROVIDER is 'openstack'. Deleting older images from OpenStack registry..."
        # Check for required OpenStack variables
        if [ -z "$OS_USERNAME" ] || [ -z "$OS_PASSWORD" ] || [ -z "$OS_AUTH_URL" ] || [ -z "$OS_PROJECT_NAME" ]; then
            echo "OpenStack credentials (OS_USERNAME, OS_PASSWORD, OS_AUTH_URL, OS_PROJECT_NAME) are not set in $ENV_FILE"
            exit 1
        fi

        # Assuming the OpenStack registry is compatible with Docker Registry HTTP API V2
        OS_REGISTRY_URL="${OS_REGISTRY_URL:-registry.example.com}"  # Replace with actual registry URL

        # Need to get a list of tags for each image and delete the older ones
        for IMAGE_NAME in "${IMAGES[@]}"; do
            echo "Processing image: $IMAGE_NAME"
            # Get list of tags
            TAGS=$(curl -s -u "$OS_USERNAME:$OS_PASSWORD" "https://$OS_REGISTRY_URL/v2/$IMAGE_NAME/tags/list" | jq -r '.tags[]')
            # Exclude the latest tag
            for TAG in $TAGS; do
                if [ "$TAG" != "$IMAGE_TAG" ]; then
                    echo "Deleting image $IMAGE_NAME:$TAG"
                    # Need to get the digest and delete by digest
                    DIGEST=$(curl -I -s -u "$OS_USERNAME:$OS_PASSWORD" "https://$OS_REGISTRY_URL/v2/$IMAGE_NAME/manifests/$TAG" -H "Accept: application/vnd.docker.distribution.manifest.v2+json" | grep Docker-Content-Digest | awk '{print $2}' | tr -d $'\r')
                    if [ -n "$DIGEST" ]; then
                        curl -s -u "$OS_USERNAME:$OS_PASSWORD" -X DELETE "https://$OS_REGISTRY_URL/v2/$IMAGE_NAME/manifests/$DIGEST"
                        echo "Deleted image $IMAGE_NAME:$TAG with digest $DIGEST"
                    else
                        echo "Failed to get digest for $IMAGE_NAME:$TAG"
                    fi
                else
                    echo "Skipping latest image $IMAGE_NAME:$TAG"
                fi
            done
        done
        ;;
    aws)
        echo "CLOUD_PROVIDER is 'aws'. Deleting older images from Amazon ECR..."
        # Check for required AWS variables
        if [ -z "$AWS_ACCOUNT_ID" ] || [ -z "$AWS_REGION" ]; then
            echo "AWS_ACCOUNT_ID and AWS_REGION must be set in $ENV_FILE"
            exit 1
        fi

        for IMAGE_NAME in "${IMAGES[@]}"; do
            echo "Processing image: $IMAGE_NAME"
            # Get list of image IDs
            IMAGE_IDS=$(aws ecr list-images --repository-name "$IMAGE_NAME" --region "$AWS_REGION" --query 'imageIds[*]' --output json)
            # Exclude the latest tag
            OLD_IMAGES=$(echo "$IMAGE_IDS" | jq -c '.[] | select(.imageTag != "'"$IMAGE_TAG"'")')
            if [ -n "$OLD_IMAGES" ]; then
                echo "Deleting old images for $IMAGE_NAME"
                aws ecr batch-delete-image --repository-name "$IMAGE_NAME" --region "$AWS_REGION" --image-ids "$OLD_IMAGES"
                echo "Deleted old images for $IMAGE_NAME"
            else
                echo "No old images to delete for $IMAGE_NAME"
            fi
        done
        ;;
    gcp)
        echo "CLOUD_PROVIDER is 'gcp'. Deleting older images from Google Container Registry..."
        # Check for required GCP variables
        if [ -z "$GCP_PROJECT_ID" ]; then
            echo "GCP_PROJECT_ID must be set in $ENV_FILE"
            exit 1
        fi

        for IMAGE_NAME in "${IMAGES[@]}"; do
            echo "Processing image: $IMAGE_NAME"
            # Get list of digests excluding the latest tag
            IMAGE_PATH="gcr.io/$GCP_PROJECT_ID/$IMAGE_NAME"
            DIGESTS=$(gcloud container images list-tags "$IMAGE_PATH" --format='get(digest)' --filter='-tags:'"$IMAGE_TAG")
            if [ -n "$DIGESTS" ]; then
                echo "Deleting old images for $IMAGE_NAME"
                for DIGEST in $DIGESTS; do
                    gcloud container images delete "$IMAGE_PATH@$DIGEST" --quiet --force-delete-tags
                    echo "Deleted image $IMAGE_PATH@$DIGEST"
                done
            else
                echo "No old images to delete for $IMAGE_NAME"
            fi
        done
        ;;
    azure)
        echo "CLOUD_PROVIDER is 'azure'. Deleting older images from Azure Container Registry..."
        # Check for required Azure variables
        if [ -z "$ACR_NAME" ]; then
            echo "ACR_NAME must be set in $ENV_FILE"
            exit 1
        fi

        for IMAGE_NAME in "${IMAGES[@]}"; do
            echo "Processing image: $IMAGE_NAME"
            # Get list of tags
            TAGS=$(az acr repository show-tags --name "$ACR_NAME" --repository "$IMAGE_NAME" --output tsv)
            # Exclude the latest tag
            for TAG in $TAGS; do
                if [ "$TAG" != "$IMAGE_TAG" ]; then
                    echo "Deleting image $IMAGE_NAME:$TAG"
                    az acr repository delete --name "$ACR_NAME" --image "$IMAGE_NAME:$TAG" --yes
                    echo "Deleted image $IMAGE_NAME:$TAG"
                else
                    echo "Skipping latest image $IMAGE_NAME:$TAG"
                fi
            done
        done
        ;;
    *)
        echo "Unsupported CLOUD_PROVIDER: $CLOUD_PROVIDER"
        exit 1
        ;;
esac

echo "run005.sh completed successfully."
exit 0
