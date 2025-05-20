#!/bin/bash
set -euo pipefail

export VAULT_ADDR=http://127.0.0.1:8200

vault server -config=/etc/vault.d/vault.hcl > /opt/vault/logs/vault_output.log 2>&1 &
VAULT_PID=$!

# Wait for Vault to become reachable
echo "Waiting for Vault to be reachable..."
for i in {1..30}; do
    if curl -s http://127.0.0.1:8200/v1/sys/health | grep -q '"initialized":'; then
        echo "Vault is responding."
        break
    fi
    echo "Waiting... ($i)"
    sleep 2
done

# Check initialization state
IS_INITIALIZED=$(vault status -format=json | jq -r '.initialized')

if [ "$IS_INITIALIZED" == "false" ]; then
    echo "Vault is not initialized. Initializing..."
    vault operator init -format=json > /opt/vault/logs/init.json
    jq -r '.root_token' /opt/vault/logs/init.json > /opt/vault/logs/.vault-token
    jq -r '.unseal_keys_b64[]' /opt/vault/logs/init.json > /opt/vault/logs/unseal-keys.txt
else
    if [ ! -s /opt/vault/logs/init.json ]; then
        echo "Vault is already initialized but init.json is empty — cannot recover."
        vault status
        exit 1
    fi
    echo "Vault is already initialized."
fi

# Load root token
VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)
export VAULT_TOKEN

# Unseal if needed
IS_SEALED=$(vault status -format=json | jq -r '.sealed')
if [ "$IS_SEALED" == "true" ]; then
    echo "Unsealing Vault..."
    if [ ! -s /opt/vault/logs/unseal-keys.txt ]; then
        echo "Missing unseal-keys.txt. Cannot unseal."
        exit 1
    fi
    while read -r key; do
        vault operator unseal "$key"
    done < /opt/vault/logs/unseal-keys.txt
else
    echo "Vault already unsealed."
fi

# Save token and config for reuse
cat <<EOF > /opt/vault/logs/vault_env.sh
export VAULT_ADDR=http://127.0.0.1:8200
export VAULT_TOKEN=$VAULT_TOKEN
EOF

# Inject application secrets
/usr/local/bin/init_vault.sh

# Wait for Vault to finish
wait $VAULT_PID
