#!/bin/bash

# Load .env file
if [ -f /.env ]; then
    . /.env
else
    echo ".env file not found!"
    exit 1
fi

# Check if Vault address and token are set
if [ -z "$VAULT_ADDR" ]; then
    echo "VAULT_ADDR is not set. Please set it and try again."
    exit 1
fi

if [ -z "$VAULT_API_ADDR" ]; then
    echo "VAULT_API_ADDR is not set. Please set it and try again."
    exit 1
fi

# Load root token from persistent file
if [ -f /opt/vault/logs/.vault-token ]; then
    export VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)
else
    echo "Vault token not found!"
    exit 1
fi

if [ -z "$VAULT_TOKEN" ]; then
    echo "VAULT_TOKEN is not set. Please set it and try again."
    exit 1
fi

# Check if required variables are set
if [ -z "$POSTGRES_PASSWORD" ] || [ -z "$POSTGRES_DB" ] || [ -z "$POSTGRES_USERNAME" ] || [ -z "$POSTGRES_PORT" ] || [ -z "$POSTGRES_HOST" ]; then
    echo "One or more required variables are not set in the .env file!"
    exit 1
fi

SECRETS_PATH="secret/postgres"

# Check Vault is reachable
curl -s "http://localhost:8200/v1/sys/health" | grep '"initialized":true' || {
  echo "Vault not initialized"; exit 1;
}

if ! curl -s -H "X-Vault-Token: $VAULT_TOKEN" "http://localhost:8200/v1/sys/seal-status" | grep -q '"sealed":false'; then
  echo "Vault is sealed, cannot proceed"
  exit 1
fi

# Ensure KV v2 enabled at `secret/`
vault secrets list | grep -q "^secret/" || vault secrets enable -path=secret kv-v2

vault token lookup | grep -q '"root"' || { echo "Token is not root."; exit 1; }

if curl -s -H "X-Vault-Token: $VAULT_TOKEN" "http://localhost:8200/v1/sys/internal/ui/mounts/$SECRETS_PATH" | grep -q "403"; then
  echo "Vault token doesn't have access to $SECRETS_PATH — check policies"
  exit 1
fi

# Confirm access before proceeding
vault kv get secret/postgres >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "Postgres secret already exists. Skipping."
    exit 0
fi

# Update Vault with PostgreSQL credentials
response=$(vault kv put secret/postgres \
    POSTGRES_HOST="$POSTGRES_HOST" \
    POSTGRES_PORT="$POSTGRES_PORT" \
    POSTGRES_DB="$POSTGRES_DB" \
    POSTGRES_USERNAME="$POSTGRES_USERNAME" \
    POSTGRES_PASSWORD="$POSTGRES_PASSWORD" 2>&1)

if echo "$response" | grep -q 'permission denied\|403'; then
    echo "Permission denied writing to Vault. Skipping secret upload."
    exit 0
elif [ $? -ne 0 ]; then
    echo "Unexpected failure while writing to Vault:"
    echo "$response"
    exit 1
fi

echo "Vault updated with PostgreSQL credentials."

cat /opt/vault/logs/vault_output.log
cat /opt/vault/logs/.vault-token

vault token lookup
vault status
