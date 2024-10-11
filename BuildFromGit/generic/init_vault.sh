#!/bin/bash

# Check if Vault address and token are set
if [ -z "$VAULT_ADDR" ]; then
    echo "VAULT_ADDR is not set. Please set it and try again."
    exit 1
fi

if [ -z "$VAULT_TOKEN" ]; then
    echo "VAULT_TOKEN is not set. Please set it and try again."
    exit 1
fi

# Load .env file
if [ -f /.env ]; then
    . /.env
else
    echo ".env file not found!"
    exit 1
fi

# Check if required variables are set
if [ -z "$POSTGRES_PASSWORD" ] || [ -z "$POSTGRES_DBNAME" ] || [ -z "$POSTGRES_USERNAME" ] || [ -z "$POSTGRES_PORT" ] || [ -z "$POSTGRES_HOST" ]; then
    echo "One or more required variables are not set in the .env file!"
    exit 1
fi

# Update Vault with PostgreSQL credentials
vault kv put secret/postgres \
    POSTGRES_HOST="$POSTGRES_HOST" \
    POSTGRES_PORT="$POSTGRES_PORT" \
    POSTGRES_DBNAME="$POSTGRES_DBNAME" \
    POSTGRES_USERNAME="$POSTGRES_USERNAME" \
    POSTGRES_PASSWORD="$POSTGRES_PASSWORD"

if [ $? -eq 0 ]; then
    echo "Vault has been updated with PostgreSQL credentials."
else
    echo "Failed to update Vault with PostgreSQL credentials."
    exit 1
fi

cat /opt/vault/logs/vault_output.log
