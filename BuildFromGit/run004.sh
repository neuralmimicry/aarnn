#!/bin/bash

# run004.sh - Deploy and execute the uploaded images on the respective cloud service or locally

echo "Starting run004.sh: Deploying and executing the container images..."

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

# Define the image names and tags
IMAGE_TAG="latest"
IMAGES=("vault-image" "postgres-image" "aarnn-image" "visualiser-image")
CONTAINER_NAMES=("vault" "postgres" "aarnn" "visualiser")

# Determine actions based on CLOUD_PROVIDER
case "$CLOUD_PROVIDER" in
    local)
        echo "CLOUD_PROVIDER is 'local'. Running the images locally using Podman..."
        # Run the images using Podman in the specified order
        for i in "${!IMAGES[@]}"; do
            IMAGE_NAME="${IMAGES[$i]}"
            CONTAINER_NAME="${CONTAINER_NAMES[$i]}"
            LOCAL_IMAGE="${IMAGE_NAME}:${IMAGE_TAG}"

            echo "Starting container $CONTAINER_NAME from image $LOCAL_IMAGE..."
            podman run -d --name "$CONTAINER_NAME" "$LOCAL_IMAGE"
            if [ $? -ne 0 ]; then
                echo "Failed to start container $CONTAINER_NAME."
                exit 1
            fi
        done
        ;;
    openstack)
        echo "CLOUD_PROVIDER is 'openstack'. Deploying the images on OpenStack..."

        # Check for required OpenStack variables
        if [ -z "$OS_USERNAME" ] || [ -z "$OS_PASSWORD" ] || [ -z "$OS_AUTH_URL" ] || [ -z "$OS_PROJECT_NAME" ] || [ -z "$OS_IMAGE_ID" ] || [ -z "$OS_FLAVOR_ID" ] || [ -z "$OS_NETWORK_ID" ]; then
            echo "OpenStack variables (OS_USERNAME, OS_PASSWORD, OS_AUTH_URL, OS_PROJECT_NAME, OS_IMAGE_ID, OS_FLAVOR_ID, OS_NETWORK_ID) are not set in $ENV_FILE"
            exit 1
        fi

        # Export OpenStack environment variables
        export OS_USERNAME OS_PASSWORD OS_AUTH_URL OS_PROJECT_NAME OS_REGION_NAME

        # Create a keypair if it doesn't exist
        KEYPAIR_NAME="${OS_KEYPAIR_NAME:-mykey}"
        if ! openstack keypair show "$KEYPAIR_NAME" >/dev/null 2>&1; then
            echo "Creating keypair $KEYPAIR_NAME..."
            openstack keypair create "$KEYPAIR_NAME" > "${KEYPAIR_NAME}.pem"
            chmod 600 "${KEYPAIR_NAME}.pem"
        else
            echo "Keypair $KEYPAIR_NAME already exists."
        fi

        # Define a security group
        SECURITY_GROUP="${OS_SECURITY_GROUP:-default}"

        # Create a cloud-init script to run the containers
        echo "Creating cloud-init script to run containers..."
        cat > cloud-init.txt << EOF
#cloud-config
packages:
 - podman
runcmd:
EOF
        for i in "${!IMAGES[@]}"; do
            IMAGE_NAME="${IMAGES[$i]}"
            CONTAINER_NAME="${CONTAINER_NAMES[$i]}"
            IMAGE_URI="${OS_REGISTRY_URL}/${OS_PROJECT_NAME}/${IMAGE_NAME}:${IMAGE_TAG}"
            echo " - podman run -d --name $CONTAINER_NAME $IMAGE_URI" >> cloud-init.txt
        done

        # Create the server instance
        SERVER_NAME="container-server-$(date +%s)"
        echo "Creating server $SERVER_NAME..."
        openstack server create \
            --image "$OS_IMAGE_ID" \
            --flavor "$OS_FLAVOR_ID" \
            --key-name "$KEYPAIR_NAME" \
            --security-group "$SECURITY_GROUP" \
            --network "$OS_NETWORK_ID" \
            --user-data cloud-init.txt \
            "$SERVER_NAME"

        echo "Server $SERVER_NAME created. Waiting for it to become ACTIVE..."
        openstack server wait --wait --timeout 600 "$SERVER_NAME"

        echo "Server $SERVER_NAME is ACTIVE."
        ;;
    aws)
        echo "CLOUD_PROVIDER is 'aws'. Deploying the images on AWS ECS..."

        # Check for required AWS variables
        if [ -z "$AWS_ACCOUNT_ID" ] || [ -z "$AWS_REGION" ] || [ -z "$AWS_CLUSTER_NAME" ]; then
            echo "AWS_ACCOUNT_ID, AWS_REGION, and AWS_CLUSTER_NAME must be set in $ENV_FILE"
            exit 1
        fi

        # Set default values
        AWS_SUBNET_ID="${AWS_SUBNET_ID}"
        AWS_SECURITY_GROUP_ID="${AWS_SECURITY_GROUP_ID}"

        if [ -z "$AWS_SUBNET_ID" ] || [ -z "$AWS_SECURITY_GROUP_ID" ]; then
            echo "AWS_SUBNET_ID and AWS_SECURITY_GROUP_ID must be set in $ENV_FILE"
            exit 1
        fi

        # Deploy each container as a separate task in the specified order
        for i in "${!IMAGES[@]}"; do
            IMAGE_NAME="${IMAGES[$i]}"
            CONTAINER_NAME="${CONTAINER_NAMES[$i]}"
            IMAGE_URI="${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com/${IMAGE_NAME}:${IMAGE_TAG}"
            TASK_DEFINITION_NAME="${IMAGE_NAME}-task-def"
            SERVICE_NAME="${IMAGE_NAME}-service"

            # Register the task definition
            echo "Registering ECS task definition for $CONTAINER_NAME..."
            cat > task-def.json << EOF
{
    "family": "${TASK_DEFINITION_NAME}",
    "networkMode": "awsvpc",
    "requiresCompatibilities": [
        "FARGATE"
    ],
    "cpu": "256",
    "memory": "512",
    "executionRoleArn": "arn:aws:iam::${AWS_ACCOUNT_ID}:role/ecsTaskExecutionRole",
    "containerDefinitions": [
        {
            "name": "${CONTAINER_NAME}",
            "image": "${IMAGE_URI}",
            "essential": true,
            "portMappings": [
                {
                    "containerPort": 80,
                    "hostPort": 80,
                    "protocol": "tcp"
                }
            ]
        }
    ]
}
EOF

            aws ecs register-task-definition --cli-input-json file://task-def.json

            # Create a service
            echo "Creating ECS service for $CONTAINER_NAME..."
            aws ecs create-service \
                --cluster "$AWS_CLUSTER_NAME" \
                --service-name "$SERVICE_NAME" \
                --task-definition "$TASK_DEFINITION_NAME" \
                --desired-count 1 \
                --launch-type "FARGATE" \
                --network-configuration "awsvpcConfiguration={subnets=[$AWS_SUBNET_ID],securityGroups=[$AWS_SECURITY_GROUP_ID],assignPublicIp=ENABLED}"

            echo "Service $SERVICE_NAME created on cluster $AWS_CLUSTER_NAME."
        done
        ;;
    gcp)
        echo "CLOUD_PROVIDER is 'gcp'. Deploying the images on Google Cloud Run..."

        # Check for required GCP variables
        if [ -z "$GCP_PROJECT_ID" ]; then
            echo "GCP_PROJECT_ID must be set in $ENV_FILE"
            exit 1
        fi

        # Deploy each container to Cloud Run in the specified order
        for i in "${!IMAGES[@]}"; do
            IMAGE_NAME="${IMAGES[$i]}"
            CONTAINER_NAME="${CONTAINER_NAMES[$i]}"
            IMAGE_URI="gcr.io/${GCP_PROJECT_ID}/${IMAGE_NAME}:${IMAGE_TAG}"

            echo "Deploying $CONTAINER_NAME to Cloud Run..."
            gcloud run deploy "$CONTAINER_NAME" \
                --image "$IMAGE_URI" \
                --platform managed \
                --region "${GCP_REGION:-us-central1}" \
                --allow-unauthenticated
        done
        ;;
    azure)
        echo "CLOUD_PROVIDER is 'azure'. Deploying the images on Azure Container Instances..."

        # Check for required Azure variables
        if [ -z "$ACR_NAME" ]; then
            echo "ACR_NAME must be set in $ENV_FILE"
            exit 1
        fi

        # Create a resource group if not exists
        RESOURCE_GROUP="${AZURE_RESOURCE_GROUP:-myResourceGroup}"
        LOCATION="${AZURE_LOCATION:-eastus}"

        az group create --name "$RESOURCE_GROUP" --location "$LOCATION"

        # Deploy each container instance in the specified order
        for i in "${!IMAGES[@]}"; do
            IMAGE_NAME="${IMAGES[$i]}"
            CONTAINER_NAME="${CONTAINER_NAMES[$i]}"
            IMAGE_URI="${ACR_NAME}.azurecr.io/${IMAGE_NAME}:${IMAGE_TAG}"

            echo "Deploying $CONTAINER_NAME to Azure Container Instances..."
            az container create \
                --resource-group "$RESOURCE_GROUP" \
                --name "$CONTAINER_NAME" \
                --image "$IMAGE_URI" \
                --registry-login-server "${ACR_NAME}.azurecr.io" \
                --ip-address public \
                --ports 80

            echo "Container instance $CONTAINER_NAME created."
        done
        ;;
    *)
        echo "Unsupported CLOUD_PROVIDER: $CLOUD_PROVIDER"
        exit 1
        ;;
esac

echo "run004.sh completed successfully."
