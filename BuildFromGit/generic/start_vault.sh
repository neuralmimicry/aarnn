#!/bin/bash
set -e

VAULT_LOG_DIR="/opt/vault/logs"
UNSEAL_KEYS_FILE="${VAULT_LOG_DIR}/unseal-keys.txt"
TOKEN_FILE="${VAULT_LOG_DIR}/.vault-token"

# Start Vault server in background
vault server -config=/etc/vault.d/vault.hcl 2>&1 | tee /opt/vault/logs/vault_output.log &
VAULT_PID=$!

# Wait for Vault to respond
echo "Waiting for Vault to be reachable..."
until curl -s http://localhost:8200/v1/sys/health >/dev/null 2>&1; do
  sleep 1
done

sleep 2

# Initialize Vault if not already initialized
if vault status | grep -q "Initialized.*false"; then
  echo "Vault is not initialized. Initializing..."
  vault operator init -key-shares=5 -key-threshold=3 \
    -format=json > "${VAULT_LOG_DIR}/init.json"
  sleep 2
  jq -r '.unseal_keys_b64[]' "${VAULT_LOG_DIR}/init.json" > "${UNSEAL_KEYS_FILE}"
  jq -r '.root_token' "${VAULT_LOG_DIR}/init.json" > "${TOKEN_FILE}"
fi

# Unseal Vault if sealed
if vault status | grep -q "Sealed.*true"; then
  echo "Unsealing Vault..."
  while IFS= read -r key; do
    vault operator unseal "$key"
    sleep 5
    if ! vault status | grep -q "Sealed.*true"; then
      break
    fi
  done < "$UNSEAL_KEYS_FILE"
fi

# Confirm it's unsealed
if vault status | grep -q "Sealed.*true"; then
  echo "Vault failed to unseal. Aborting."
  exit 1
fi

echo "Vault is initialized and unsealed."

# Export token so init script can use it
export VAULT_TOKEN=$(< "$TOKEN_FILE")

# Run post-init script
if [ -x /usr/local/bin/init_vault.sh ]; then
  echo "Running init_vault.sh..."
  /usr/local/bin/init_vault.sh || echo "init_vault.sh failed (non-fatal)"
fi

curl -H "X-Vault-Token: $VAULT_TOKEN" \
  http://localhost:8200/v1/secret/data/postgres

# Wait on Vault PID
wait $VAULT_PID
