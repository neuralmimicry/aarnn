#!/bin/bash

# run002.sh - Install cloud client and login based on CLOUD_PROVIDER from .env

echo "Starting run002.sh: Determining cloud provider and installing client..."

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

# Define the Ansible playbook directory
PLAYBOOK_DIR="./BuildFromGit/ansible"

# Function to run Ansible playbook
run_playbook() {
    local playbook="$1"
    if [ ! -f "$PLAYBOOK_DIR/$playbook" ]; then
        echo "Playbook $PLAYBOOK_DIR/$playbook not found!"
        exit 1
    fi
    echo "Running Ansible playbook: $playbook"
    ansible-playbook -i localhost, "$PLAYBOOK_DIR/$playbook" --connection=local --become
    if [ $? -ne 0 ]; then
        echo "Ansible playbook $playbook failed."
        exit 1
    else
        echo "Ansible playbook $playbook completed successfully."
    fi
}

# Determine actions based on CLOUD_PROVIDER
case "$CLOUD_PROVIDER" in
    local)
        echo "CLOUD_PROVIDER is set to 'local'. Installing Podman..."
        run_playbook "install_podman.yml"
        ;;
    openstack)
        echo "CLOUD_PROVIDER is set to 'openstack'. Installing OpenStack client..."
        run_playbook "install_openstack_client.yml"
        echo "Logging in to OpenStack..."
        # Perform OpenStack login
        if [ -z "$OS_USERNAME" ] || [ -z "$OS_PASSWORD" ] || [ -z "$OS_AUTH_URL" ]; then
            echo "OpenStack credentials (OS_USERNAME, OS_PASSWORD, OS_AUTH_URL) are not set in $ENV_FILE"
            exit 1
        fi
        export OS_USERNAME OS_PASSWORD OS_AUTH_URL
        echo "OpenStack credentials exported."
        ;;
    aws)
        echo "CLOUD_PROVIDER is set to 'aws'. Installing AWS CLI..."
        run_playbook "install_aws_cli.yml"
        echo "Logging in to AWS..."
        # Configure AWS CLI
        if [ -n "$AWS_ACCESS_KEY_ID" ] && [ -n "$AWS_SECRET_ACCESS_KEY" ]; then
            aws configure set aws_access_key_id "$AWS_ACCESS_KEY_ID"
            aws configure set aws_secret_access_key "$AWS_SECRET_ACCESS_KEY"
            aws configure set default.region "${AWS_DEFAULT_REGION:-us-east-1}"
            echo "AWS CLI configured with provided credentials."
        else
            echo "AWS credentials not found in $ENV_FILE. Please enter them interactively."
            aws configure
        fi
        ;;
    gcp)
        echo "CLOUD_PROVIDER is set to 'gcp'. Installing Google Cloud SDK..."
        run_playbook "install_gcp_sdk.yml"
        echo "Logging in to Google Cloud..."
        # Perform GCP login
        gcloud auth login
        ;;
    azure)
        echo "CLOUD_PROVIDER is set to 'azure'. Installing Azure CLI..."
        run_playbook "install_azure_cli.yml"
        echo "Logging in to Azure..."
        # Perform Azure login
        az login
        ;;
    *)
        echo "Unknown CLOUD_PROVIDER: $CLOUD_PROVIDER"
        exit 1
        ;;
esac

echo "run002.sh completed successfully."
