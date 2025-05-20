#!/bin/bash
set -euo pipefail

export VAULT_ADDR=http://vault:8200

# Start Vault in the background
vault server -config=/etc/vault.d/vault.hcl > /opt/vault/logs/vault_output.log 2>&1 &
VAULT_PID=$!

# Wait for Vault to respond to /sys/health
echo "Waiting for Vault to respond..."
for i in {1..30}; do
  status=$(curl -s http://vault:8200/v1/sys/health || echo "")
  if echo "$status" | grep -q '"initialized":'; then
    echo "Vault is reachable."
    break
  fi
  sleep 1
done

# Check init state
if ! vault status | grep -q "Initialized.*true"; then
  echo "Vault is not initialized. Initializing..."
  vault operator init -format=json > /opt/vault/logs/init.json
  jq -r '.root_token' /opt/vault/logs/init.json > /opt/vault/logs/.vault-token
  jq -r '.unseal_keys_b64[]' /opt/vault/logs/init.json > /opt/vault/logs/unseal-keys.txt
else
  echo "Vault already initialized."
fi

# Unseal Vault if sealed
if vault status | grep -q "Sealed.*true"; then
  echo "Vault is sealed. Unsealing..."
  for key in $(cat /opt/vault/logs/unseal-keys.txt); do
    vault operator unseal "$key" || true
    sleep 1
    if ! vault status | grep -q "Sealed.*true"; then
      echo "Vault is now unsealed."
      break
    fi
  done
fi

# Double-check before continuing
if vault status | grep -q "Sealed.*true"; then
  echo "Vault is still sealed after attempted unseal. Aborting init script."
  exit 1
fi

# Load token
export VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)

if [ -x /usr/local/bin/init_vault.sh ]; then
  echo "Running init_vault.sh..."
  /usr/local/bin/init_vault.sh || echo "init_vault.sh failed (non-fatal)"
fi

echo "Vault ready."
wait $VAULT_PID
