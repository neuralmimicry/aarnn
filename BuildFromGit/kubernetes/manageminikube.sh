#!/bin/bash

# Purpose: Manage Kubernetes deployments and services for PostgreSQL, Vault, Aarnn, and Visualiser

# Load environment variables
set -a
. ./.env
set +a

# Function to check required environment variables
check_variables() {
    required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DB" "POSTGRES_PORT_EXPOSE" "PULSE_SINK" "PULSE_SOURCE" "XDG_RUNTIME_DIR" "POSTGRES_HOST" "POSTGRES_PORT")
    for var in "${required_vars[@]}"; do
        value=$(echo "${!var}" | sed 's/^"//; s/"$//')
        export "$var"="$value"
        if [ -z "$value" ]; then
            echo "Error: Environment variable $var is not set."
            exit 1
        fi
    done
}

# Function to save current UFW rules
save_ufw_rules() {
    sudo rm -f /etc/ufw/*.202*
    sudo ufw status numbered > /tmp/ufw.rules.before
    # Remove headings and format the file
    awk 'NR>3 {print substr($0, index($0,$3))}' /tmp/ufw.rules.before | sed 's/\[//g;s/\]//g' | sed '/^-/d' > /tmp/ufw.rules.before.formatted

    # Format rules for re-importing
    sed -i -E 's/(^.*)(ALLOW IN)(.*$)/allow \1\3/g; s/(^.*)(ALLOW OUT)(.*$)/allow \1\3/g' /tmp/ufw.rules.before.formatted
}

# Function to restore UFW rules
restore_ufw_rules() {
    if [ -f /tmp/ufw.rules.before.formatted ]; then
        sudo ufw --force reset
        sudo ufw --force enable
        sudo bash -c 'while read -r line; do sudo ufw $line; done < /tmp/ufw.rules.before.formatted'
    fi
}

# Function to apply UFW rules for Minikube
apply_ufw_rules() {
    sudo ufw allow 16443/tcp
    sudo ufw allow 19001/tcp
    sudo ufw allow 8200/tcp
    sudo ufw allow 8443/tcp
    sudo ufw allow 5432/tcp
    sudo ufw allow 5000/tcp
    sudo ufw allow 30000:48000/tcp
    sudo ufw allow 53/udp
    sudo ufw allow 53/tcp
    sudo ufw allow 8285/udp
    sudo ufw allow 8472/udp
}

# Function to ensure Minikube is running properly
ensure_minikube_running() {
    echo "Ensuring Minikube is running..."
    minikube start --driver=podman --force-systemd=true
    sleep 10

    # Check Minikube status
    if ! minikube status --format=json | jq -e '.Host == "Running" and .Kubelet == "Running" and .APIServer == "Running"' > /dev/null; then
        echo "Minikube is not running properly"
        exit 1
    fi

    sleep 15

    # Enable DNS and storage if not already enabled
    minikube addons enable dns
    sleep 10
    minikube addons enable storage-provisioner
    sleep 10

    # Ensure local registry is running
    minikube addons enable registry
    sleep 10
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
        if [[ $($command 2>&1) == *"NotFound"* ]]; then
            echo "Resource not found, treating as success"
            return 0
        fi
        echo "Attempt $i failed. Retrying in 15 seconds..."
        sleep 15
    done

    echo "Command failed after $retries attempts."
    return 1
}

# Function to check the status of the containers
check_containers() {
    echo "Checking pod status..."
    if ! retry_command 5 kubectl get pods; then
        echo "Failed to get pods status"
    fi

    echo "Checking service endpoints..."
    if ! retry_command 5 kubectl get svc; then
        echo "Failed to get services status"
    fi

    if ! retry_command 5 kubectl get endpoints; then
        echo "Failed to get endpoints status"
    fi

    echo "Verifying Vault status..."
    VAULT_POD=$(kubectl get pods --selector=app=vault -o jsonpath='{.items[0].metadata.name}')
    if [ -n "$VAULT_POD" ]; then
        kubectl exec -it "$VAULT_POD" -- vault status
    else
        echo "Vault pod not found"
    fi

    echo "Verifying PostgreSQL connection..."
    POSTGRES_POD=$(kubectl get pods --selector=app=postgres -o jsonpath='{.items[0].metadata.name}')
    if [ -n "$POSTGRES_POD" ]; then
        kubectl exec -it "$POSTGRES_POD" -- psql -U "$POSTGRES_USERNAME" -d "$POSTGRES_DB" -c "\l"
    else
        echo "PostgreSQL pod not found"
    fi

    echo "Checking logs for each app..."
    apps=("postgres" "vault" "aarnn" "visualiser")

    for app in "${apps[@]}"; do
        echo "Checking logs for $app..."
        POD_NAME=$(kubectl get pods --selector=app="$app" -o jsonpath='{.items[0].metadata.name}')
        if [ -n "$POD_NAME" ]; then
            kubectl logs "$POD_NAME"
        else
            echo "Pod for $app not found"
        fi
    done
}

# Function to check the status of deployments
check_deployments() {
    echo "Checking deployment status..."
    apps=("postgres" "vault" "aarnn" "visualiser")

    for app in "${apps[@]}"; do
        echo "Describing deployment for $app..."
        DEPLOY_NAME=$(kubectl get deployments --selector=app="$app" -o jsonpath='{.items[0].metadata.name}')
        if [ -n "$DEPLOY_NAME" ]; then
            kubectl describe deployment "$DEPLOY_NAME"
            echo "Checking events for deployment $DEPLOY_NAME..."
            kubectl get events --field-selector involvedObject.name="$DEPLOY_NAME" --sort-by='.lastTimestamp'
        else
            echo "Deployment for $app not found"
        fi
    done
}

# Function to list images used by the deployments
list_images() {
    echo "Detecting images used by the deployments..."
    apps=("vault" "postgres" "aarnn" "visualiser")
    all_images_found=true

    for app in "${apps[@]}"; do
        echo "Images for $app:"
        IMAGES=$(kubectl get pods --selector=app="$app" -o jsonpath="{.items[*].spec.containers[*].image}" | tr -s '[[:space:]]' '\n' | sort | uniq)
        if [ -n "$IMAGES" ]; then
            echo "$IMAGES"
        else
            echo "Image for $app not found"
            all_images_found=false
        fi
    done

    if ! $all_images_found; then
        echo "One or more images are missing. Exiting."
        exit 1
    else
        echo "All images found."
    fi
}

# Function to save images from Podman to tar files
save_images() {
    echo "Saving images from Podman..."
    images=("vault" "postgres" "aarnn" "visualiser")
    for image in "${images[@]}"; do
        podman save -o "${image}.tar" "${image}"
    done
}

# Function to load images into Minikube
load_images() {
    echo "Loading images into Minikube..."
    images=("vault" "postgres" "aarnn" "visualiser")
    for image in "${images[@]}"; do
        podman load -i "${image}.tar"
    done
    confirm_images_loaded
}

# Function to confirm images are loaded into Minikube
confirm_images_loaded() {
    echo "Confirming images are loaded into Minikube..."
    images=("vault" "postgres" "aarnn" "visualiser")
    all_images_found=true

    for image in "${images[@]}"; do
        if podman images | grep -q "${image}"; then
            echo "Image ${image} is present"
        else
            echo "Image ${image} is missing"
            all_images_found=false
        fi
    done

    if ! $all_images_found; then
        echo "One or more images are missing. Exiting."
        exit 1
    fi
}

# Function to check if the local registry is accessible
check_registry_accessible() {
    for i in {1..10}; do
        if curl -k http://localhost:5000/v2/; then
            echo "Local registry is accessible"
            return 0
        fi
        echo "Local registry is not accessible, retrying in 10 seconds..."
        sleep 10
        if [ "$i" -eq 10 ]; then
            echo "Local registry is not accessible after multiple attempts"
            return 1
        fi
    done
}

# Function to tag and push images to local registry
push_images_to_local_registry() {
    echo "Pushing images to local registry..."
    images=("vault" "postgres" "aarnn" "visualiser")
    registry="localhost:5000"

    for image in "${images[@]}"; do
        podman tag "${image}" "${registry}/${image}"
        retry_command 5 podman push "${registry}/${image}"
    done
}

# Function to start Kubernetes deployments and services
start_containers() {
    # Ensure Minikube is running
    ensure_minikube_running

    # Check environment variables
    check_variables

    # Save current UFW rules and apply necessary rules for Minikube
    save_ufw_rules
    apply_ufw_rules

    sleep 5

    # Check if the required images are available
    list_images

    # Encode secrets to base64
    POSTGRES_USERNAME_BASE64=$(echo -n "${POSTGRES_USERNAME}" | base64)
    POSTGRES_PASSWORD_BASE64=$(echo -n "${POSTGRES_PASSWORD}" | base64)

    # Replace placeholders in secret-postgres.yaml with base64 encoded values
    sed -i "s|\${POSTGRES_USERNAME_BASE64}|${POSTGRES_USERNAME_BASE64}|g" secret-postgres.yaml
    sed -i "s|\${POSTGRES_PASSWORD_BASE64}|${POSTGRES_PASSWORD_BASE64}|g" secret-postgres.yaml

    sleep 20

    echo "Applying Kubernetes configurations..."

    files=("deployment-vault.yaml" "service-vault.yaml" "secret-postgres.yaml" "config-postgres.yaml" "deployment-postgres.yaml" "service-postgres.yaml" "deployment-aarnn.yaml" "service-aarnn.yaml" "deployment-visualiser.yaml" "service-visualiser.yaml")

    for file in "${files[@]}"; do
        if ! retry_command 5 kubectl apply -f "$file"; then
            echo "Failed to apply $file"
            exit 1
        fi
        sleep 20
    done

    echo "Kubernetes deployments and services have been successfully applied. Pausing for 60 seconds to allow services to start..."
    sleep 60

    # Check the status of the containers
    check_deployments
    check_containers
}

# Function to stop Kubernetes deployments and services
stop_containers() {
    echo "Deleting Kubernetes deployments and services..."

    files=("service-visualiser.yaml" "deployment-visualiser.yaml" "service-aarnn.yaml" "deployment-aarnn.yaml" "service-postgres.yaml" "deployment-postgres.yaml" "config-postgres.yaml" "secret-postgres.yaml" "service-vault.yaml" "deployment-vault.yaml")

    for file in "${files[@]}"; do
        if ! retry_command 5 kubectl delete -f "$file"; then
            echo "Failed to delete $file"
        fi
        sleep 5
    done

    # Restore UFW rules to their original state
    restore_ufw_rules
}

# Function to show logs for a specific Kubernetes pod
show_logs() {
    if [[ "$1" == "" ]]; then
        echo "Please specify the deployment name for logs (postgres, vault, aarnn, or visualiser)"
    else
        pod_name=$(kubectl get pods --selector=app=$1 --output=jsonpath={.items..metadata.name})
        if [[ "$pod_name" == "" ]]; then
            echo "No running pods found for deployment $1"
        else
            echo "Fetching logs from pod $pod_name..."
            kubectl logs -f "$pod_name"
        fi
    fi
}

# Main script logic
case "$1" in
    save)
        save_images
        ;;
    load)
        ensure_minikube_running
        check_variables
        save_ufw_rules
        apply_ufw_rules
        sleep 5
        load_images
        ;;
    push)
        ensure_minikube_running
        check_variables
        save_ufw_rules
        apply_ufw_rules
        sleep 5
        if check_registry_accessible; then
            push_images_to_local_registry
        else
            echo "Local registry is not accessible. Exiting."
            exit 1
        fi
        ;;
    start)
        ensure_minikube_running
        check_variables
        save_ufw_rules
        apply_ufw_rules
        sleep 5
        list_images
        start_containers
        ;;
    stop)
        stop_containers
        ;;
    logs)
        show_logs "$2"
        ;;
    *)
        echo "Usage: $0 {save|load|push|start|stop|logs <deployment_name>}"
        echo "  save - Save images from Podman to tar files"
        echo "  load - Load images into Minikube from tar files"
        echo "  push - Tag and push images to local registry"
        echo "  start - Starts all Kubernetes deployments and services"
        echo "  stop - Stops all Kubernetes deployments and services"
        echo "  logs <deployment_name> - Tails logs from the specified deployment (postgres, vault, aarnn, or visualiser)"
        ;;
esac
