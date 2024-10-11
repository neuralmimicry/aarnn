#!/bin/bash

# Purpose: Manage Kubernetes deployments and services for PostgreSQL, Vault, Aarnn, and Visualiser

# Load environment variables
set -a
. ./.env
set +a

# Function to check required environment variables
check_variables() {
    required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DBNAME" "POSTGRES_PORT_EXPOSE" "PULSE_SINK" "PULSE_SOURCE" "XDG_RUNTIME_DIR" "POSTGRES_HOST" "POSTGRES_PORT")
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

# Function to apply UFW rules for MicroK8s
apply_ufw_rules() {
    sudo ufw allow 16443/tcp
    sudo ufw allow 19001/tcp
    sudo ufw allow 8200/tcp
    sudo ufw allow 5460/tcp
    sudo ufw allow 30000:32767/tcp
    sudo ufw allow 53/udp
    sudo ufw allow 53/tcp
    sudo ufw allow 8285/udp
    sudo ufw allow 8472/udp
}

# Function to ensure MicroK8s is running properly
ensure_microk8s_running() {
    echo "Ensuring MicroK8s is running..."
    sudo microk8s start
    sleep 10
    sudo microk8s status --wait-ready
    sleep 10

    # Check MicroK8s services
    sudo microk8s inspect 2>&1 | tee /tmp/microk8s-inspect.log
    if grep -q "error" /tmp/microk8s-inspect.log; then
        echo "MicroK8s services are not running properly"
        exit 1
    fi

    sleep 15

    # Retry logic to ensure API server is reachable
    for i in {1..10}; do
        if curl -k https://127.0.0.1:16443/healthz; then
            echo "Kubernetes API server is reachable"
            break
        fi
        echo "Kubernetes API server is not reachable, retrying in 10 seconds..."
        sleep 10
        if [ "$i" -eq 10 ]; then
            echo "Kubernetes API server is not reachable at https://127.0.0.1:16443 after multiple attempts"
            exit 1
        fi
    done

    # Check Kubernetes Configuration
    if ! retry_command 5 sudo microk8s kubectl config view; then
        echo "Failed to view kubectl config"
        exit 1
    fi

    # shellcheck disable=SC2024
    sudo microk8s kubectl config view --raw > "$HOME"/.kube/config
    export KUBECONFIG=$HOME/.kube/config

    if ! retry_command 5 sudo microk8s kubectl config current-context; then
        echo "Failed to get current kubectl context"
        exit 1
    fi

    # Enable DNS if not already enabled
    sudo microk8s enable dns
    sleep 10
    # Enable storage if not already enabled
    sudo microk8s enable storage
    sleep 10
    # Enable local registry if not already enabled
    sudo microk8s enable registry
    sleep 10
    sudo microk8s kubectl get all -n container-registry
    sleep 10
    sudo microk8s kubectl describe namespace container-registry
    sleep 10
    sudo microk8s kubectl apply -f registry-deployment.yaml
    sleep 10
    sudo microk8s kubectl get pods -n container-registry
    sleep 10
    sudo microk8s kubectl get svc -n container-registry
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
    if ! retry_command 5 microk8s kubectl get pods; then
        echo "Failed to get pods status"
    fi

    echo "Checking service endpoints..."
    if ! retry_command 5 microk8s kubectl get svc; then
        echo "Failed to get services status"
    fi

    if ! retry_command 5 microk8s kubectl get endpoints; then
        echo "Failed to get endpoints status"
    fi

    echo "Verifying Vault status..."
    VAULT_POD=$(microk8s kubectl get pods --selector=app=vault -o jsonpath='{.items[0].metadata.name}')
    if [ -n "$VAULT_POD" ]; then
        microk8s kubectl exec -it "$VAULT_POD" -- vault status
    else
        echo "Vault pod not found"
    fi

    echo "Verifying PostgreSQL connection..."
    POSTGRES_POD=$(microk8s kubectl get pods --selector=app=postgres -o jsonpath='{.items[0].metadata.name}')
    if [ -n "$POSTGRES_POD" ]; then
        microk8s kubectl exec -it "$POSTGRES_POD" -- psql -U "$POSTGRES_USERNAME" -d "$POSTGRES_DBNAME" -c "\l"
    else
        echo "PostgreSQL pod not found"
    fi

    echo "Checking logs for each app..."
    apps=("postgres" "vault" "aarnn" "visualiser")

    for app in "${apps[@]}"; do
        echo "Checking logs for $app..."
        POD_NAME=$(microk8s kubectl get pods --selector=app="$app" -o jsonpath='{.items[0].metadata.name}')
        if [ -n "$POD_NAME" ]; then
            microk8s kubectl logs "$POD_NAME"
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
        DEPLOY_NAME=$(microk8s kubectl get deployments --selector=app="$app" -o jsonpath='{.items[0].metadata.name}')
        if [ -n "$DEPLOY_NAME" ]; then
            microk8s kubectl describe deployment "$DEPLOY_NAME"
            echo "Checking events for deployment $DEPLOY_NAME..."
            microk8s kubectl get events --field-selector involvedObject.name="$DEPLOY_NAME" --sort-by='.lastTimestamp'
        else
            echo "Deployment for $app not found"
        fi
    done
}

# Function to list images used by the deployments
list_images() {
    # List all images used by the pods
    microk8s kubectl get pods --all-namespaces -o jsonpath="{.items[*].spec.containers[*].image}" | tr -s '[[:space:]]' '\n' | sort | uniq

    # List images used by the deployments
    echo "Detecting images used by the deployments..."
    apps=("vault" "postgres" "aarnn" "visualiser")
    all_images_found=true

    for app in "${apps[@]}"; do
        echo "Images for $app:"
        IMAGES=$(microk8s kubectl get pods --selector=app="$app" -o jsonpath="{.items[*].spec.containers[*].image}" | tr -s '[[:space:]]' '\n' | sort | uniq)
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

# Function to load images into MicroK8s
load_images() {
    echo "Loading images into MicroK8s..."
    images=("vault" "postgres" "aarnn" "visualiser")
    for image in "${images[@]}"; do
        sudo microk8s ctr image import "${image}.tar"
    done
    confirm_images_loaded
}

# Function to confirm images are loaded into MicroK8s
confirm_images_loaded() {
    echo "Confirming images are loaded into MicroK8s..."
    images=("vault" "postgres" "aarnn" "visualiser")
    all_images_found=true

    for image in "${images[@]}"; do
        if sudo microk8s ctr images list | grep -q "${image}"; then
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
        if curl -k https://localhost:32000/v2/; then
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
    registry="localhost:32000"

    for image in "${images[@]}"; do
        podman tag "${image}" "${registry}/${image}"
        retry_command 5 podman push "${registry}/${image}"
    done
}

# Function to start Kubernetes deployments and services
start_containers() {
    # Ensure MicroK8s is running
    ensure_microk8s_running

    # Check environment variables
    check_variables

    # Save current UFW rules and apply necessary rules for MicroK8s
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

    if ! retry_command 5 microk8s kubectl apply -f deployment-vault.yaml; then
        echo "Failed to apply deployment-vault.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f service-vault.yaml; then
        echo "Failed to apply service-vault.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f secret-postgres.yaml --validate=false; then
        echo "Failed to apply secret-postgres.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f config-postgres.yaml --validate=false; then
        echo "Failed to apply config-postgres.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f deployment-postgres.yaml --validate=false; then
        echo "Failed to apply deployment-postgres.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f service-postgres.yaml --validate=false; then
        echo "Failed to apply service-postgres.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f deployment-aarnn.yaml --validate=false; then
        echo "Failed to apply deployment-aarnn.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f service-aarnn.yaml --validate=false; then
        echo "Failed to apply service-aarnn.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f deployment-visualiser.yaml --validate=false; then
        echo "Failed to apply deployment-visualiser.yaml"
        exit 1
    fi

    sleep 20

    if ! retry_command 5 microk8s kubectl apply -f service-visualiser.yaml --validate=false; then
        echo "Failed to apply service-visualiser.yaml"
        exit 1
    fi

    echo "Kubernetes deployments and services have been successfully applied. Pausing for 60 seconds to allow services to start..."
    sleep 60

    # Check the status of the containers
    check_deployments
    check_containers
}

# Function to stop Kubernetes deployments and services
stop_containers() {
    echo "Deleting Kubernetes deployments and services..."

    retry_command 5 microk8s kubectl delete -f service-visualiser.yaml || echo "Failed to delete service-visualiser.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f deployment-visualiser.yaml || echo "Failed to delete deployment-visualiser.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f service-aarnn.yaml || echo "Failed to delete service-aarnn.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f deployment-aarnn.yaml || echo "Failed to delete deployment-aarnn.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f service-postgres.yaml || echo "Failed to delete service-postgres.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f deployment-postgres.yaml || echo "Failed to delete deployment-postgres.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f config-postgres.yaml || echo "Failed to delete config-postgres.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f secret-postgres.yaml || echo "Failed to delete secret-postgres.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f service-vault.yaml || echo "Failed to delete service-vault.yaml"

    sleep 5

    retry_command 5 microk8s kubectl delete -f deployment-vault.yaml || echo "Failed to delete deployment-vault.yaml"

    sleep 5

    # Restore UFW rules to their original state
    restore_ufw_rules
}

# Function to show logs for a specific Kubernetes pod
show_logs() {
    if [[ "$1" == "" ]]; then
        echo "Please specify the deployment name for logs (postgres, vault, aarnn, or visualiser)"
    else
        pod_name=$(microk8s kubectl get pods --selector=app=$1 --output=jsonpath={.items..metadata.name})
        if [[ "$pod_name" == "" ]]; then
            echo "No running pods found for deployment $1"
        else
            echo "Fetching logs from pod $pod_name..."
            microk8s kubectl logs -f "$pod_name"
        fi
    fi
}

# Main script logic
case "$1" in
    save)
        save_images
        ;;
    load)
        ensure_microk8s_running
        check_variables
        save_ufw_rules
        apply_ufw_rules
        sleep 5
        load_images
        ;;
    push)
        ensure_microk8s_running
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
        ensure_microk8s_running
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
        echo "  load - Load images into MicroK8s from tar files"
        echo "  push - Tag and push images to local registry"
        echo "  start - Starts all Kubernetes deployments and services"
        echo "  stop - Stops all Kubernetes deployments and services"
        echo "  logs <deployment_name> - Tails logs from the specified deployment (postgres, vault, aarnn, or visualiser)"
        ;;
esac
