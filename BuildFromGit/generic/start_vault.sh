#!/bin/bash

source /app/.env

# Check if VAULT_DEV_ROOT_TOKEN_ID is set
if [ -n "$VAULT_DEV_ROOT_TOKEN_ID" ]; then
    echo "Starting Vault with pre-existing token."
    export VAULT_DEV_ROOT_TOKEN_ID
    # Start Vault in development mode with the specified or default token
    vault server -dev \
        -dev-root-token-id="${VAULT_DEV_ROOT_TOKEN_ID:-root}" \
        -dev-listen-address="${VAULT_DEV_LISTEN_ADDRESS:-0.0.0.0:8200}" \
        -log-level=info | tee /opt/vault/logs/vault_output.log 2>&1 &
    VAULT_PID=$!
else
    echo "Starting Vault without a pre-existing token."
    # Start Vault in development mode with the specified or default token
    vault server -dev \
        -log-level=info | tee /opt/vault/logs/vault_output.log 2>&1 &
    VAULT_PID=$!
fi

# Wait for a few seconds to allow Vault to initialize
sleep 5

# Debugging
cat /opt/vault/logs/vault_output.log

# Check if Vault is initialized and running
for i in {1..60}; do
    if grep -q 'Root Token' /opt/vault/logs/vault_output.log; then
        echo "Vault is initialized and running."
        break
    else
        echo "Waiting for Vault to initialize... ($i)"
        sleep 2
    fi

    if [ "$i" -eq 60 ]; then
        echo "Vault did not initialize in time."
        cat /opt/vault/logs/vault_output.log
        exit 1
    fi
done

# Extract the first matching VAULT_ADDR and truncate if longer than 25 characters
# VAULT_ADDR=$(grep -m 1 '127.0.0.1' /opt/vault/logs/vault_output.log | grep -o 'http://127.0.0.1:[0-9]*' | cut -c 1-25)
VAULT_ADDR=http://vault:8200
VAULT_TOKEN=$(grep 'Root Token' /opt/vault/logs/vault_output.log | awk '{print $3}')

# Export VAULT_ADDR and VAULT_TOKEN as environment variables
export VAULT_ADDR
export VAULT_TOKEN

echo "Vault server started with address: $VAULT_ADDR"
echo "Vault root token: $VAULT_TOKEN"

# Save these variables to a file for later use
cat <<EOL > /opt/vault/logs/vault_env.sh
VAULT_ADDR=$VAULT_ADDR
VAULT_TOKEN=$VAULT_TOKEN
EOL

# Initialize Vault with secrets
/usr/local/bin/init_vault.sh

# Wait for the Vault server process to exit
wait $VAULT_PID
