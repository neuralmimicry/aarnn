#!/bin/bash

# run004.sh - Deploy and execute the uploaded image on the respective cloud service or locally

echo "Starting run004.sh: Deploying and executing the container image..."

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

# Determine actions based on CLOUD_PROVIDER
case "$CLOUD_PROVIDER" in
    local)
        echo "CLOUD_PROVIDER is 'local'. Running the image locally using Podman..."
        # Run the image using Podman
        podman run --rm -it "${IMAGE_NAME}:${IMAGE_TAG}"
        ;;
    openstack)
        echo "CLOUD_PROVIDER is 'openstack'. Deploying the image on OpenStack..."

        # Check for required OpenStack variables
        if [ -z "$OS_USERNAME" ] || [ -z "$OS_PASSWORD" ] || [ -z "$OS_AUTH_URL" || -z "$OS_PROJECT_NAME" ] || [ -z "$OS_IMAGE_ID" ] || [ -z "$OS_FLAVOR_ID" ] || [ -z "$OS_NETWORK_ID" ]; then
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

        # Create a cloud-init script to run the container
        cat > cloud-init.txt << EOF
#cloud-config
packages:
 - podman
runcmd:
 - podman run -d --name mycontainer ${OS_REGISTRY_URL}/${OS_PROJECT_NAME}/${IMAGE_NAME}:${IMAGE_TAG}
EOF

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
        echo "CLOUD_PROVIDER is 'aws'. Deploying the image on AWS ECS..."

        # Check for required AWS variables
        if [ -z "$AWS_ACCOUNT_ID" ] || [ -z "$AWS_REGION" ] || [ -z "$AWS_CLUSTER_NAME" ]; then
            echo "AWS_ACCOUNT_ID, AWS_REGION, and AWS_CLUSTER_NAME must be set in $ENV_FILE"
            exit 1
        fi

        # Set default values
        AWS_TASK_DEFINITION_NAME="${AWS_TASK_DEFINITION_NAME:-my-task-def}"
        AWS_SERVICE_NAME="${AWS_SERVICE_NAME:-my-service}"
        AWS_LAUNCH_TYPE="${AWS_LAUNCH_TYPE:-FARGATE}"
        AWS_SUBNET_ID="${AWS_SUBNET_ID}"
        AWS_SECURITY_GROUP_ID="${AWS_SECURITY_GROUP_ID}"

        if [ -z "$AWS_SUBNET_ID" ] || [ -z "$AWS_SECURITY_GROUP_ID" ]; then
            echo "AWS_SUBNET_ID and AWS_SECURITY_GROUP_ID must be set in $ENV_FILE"
            exit 1
        fi

        # Define the ECR image URI
        IMAGE_URI="${AWS_ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com/${IMAGE_NAME}:${IMAGE_TAG}"

        # Register the task definition
        echo "Registering ECS task definition..."
        cat > task-def.json << EOF
{
    "family": "${AWS_TASK_DEFINITION_NAME}",
    "networkMode": "awsvpc",
    "requiresCompatibilities": [
        "${AWS_LAUNCH_TYPE}"
    ],
    "cpu": "256",
    "memory": "512",
    "executionRoleArn": "arn:aws:iam::${AWS_ACCOUNT_ID}:role/ecsTaskExecutionRole",
    "containerDefinitions": [
        {
            "name": "${IMAGE_NAME}",
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
        echo "Creating ECS service..."
        aws ecs create-service \
            --cluster "$AWS_CLUSTER_NAME" \
            --service-name "$AWS_SERVICE_NAME" \
            --task-definition "$AWS_TASK_DEFINITION_NAME" \
            --desired-count 1 \
            --launch-type "$AWS_LAUNCH_TYPE" \
            --network-configuration "awsvpcConfiguration={subnets=[$AWS_SUBNET_ID],securityGroups=[$AWS_SECURITY_GROUP_ID],assignPublicIp=ENABLED}"

        echo "Service $AWS_SERVICE_NAME created on cluster $AWS_CLUSTER_NAME."
        ;;
    gcp)
        echo "CLOUD_PROVIDER is 'gcp'. Deploying the image on Google Cloud Run..."

        # Check for required GCP variables
        if [ -z "$GCP_PROJECT_ID" ]; then
            echo "GCP_PROJECT_ID must be set in $ENV_FILE"
            exit 1
        fi

        # Define the image URI
        IMAGE_URI="gcr.io/${GCP_PROJECT_ID}/${IMAGE_NAME}:${IMAGE_TAG}"

        # Deploy to Cloud Run
        echo "Deploying the image to Cloud Run..."
        gcloud run deploy "${IMAGE_NAME}" \
            --image "${IMAGE_URI}" \
            --platform managed \
            --region us-central1 \
            --allow-unauthenticated
        ;;
    azure)
        echo "CLOUD_PROVIDER is 'azure'. Deploying the image on Azure Container Instances..."

        # Check for required Azure variables
        if [ -z "$AZ_RESOURCE_GROUP" ] || [ -z "$AZ_LOCATION" ]; then
            echo "AZ_RESOURCE_GROUP and AZ_LOCATION must be set in $ENV_FILE"
            exit 1
        fi

        # Define the image URI
        IMAGE_URI="${IMAGE_NAME}:${IMAGE_TAG}"

        # Create a resource group
        echo "Creating resource group $AZ_RESOURCE_GROUP..."
        az group create --name "$AZ_RESOURCE_GROUP" --location "$AZ_LOCATION"

        # Create the container instance
        echo "Creating container instance..."
        az container create \
            --resource-group "$AZ_RESOURCE_GROUP" \
            --name "${IMAGE_NAME}" \
            --image "${IMAGE_URI}" \
            --cpu 1 \
            --memory 1.5 \
            --restart-policy Never
        ;;
    *)
        echo "Unsupported CLOUD_PROVIDER: $CLOUD_PROVIDER"
        exit 1
        ;;
esac

