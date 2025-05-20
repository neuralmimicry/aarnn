#!/bin/bash
set -e

export VAULT_ADDR=http://127.0.0.1:8200

vault server -config=/etc/vault.d/vault.hcl -log-level=debug > /opt/vault/logs/vault_output.log 2>&1 &
VAULT_PID=$!

echo "Waiting for Vault to be reachable..."
for i in {1..30}; do
    if curl -s http://127.0.0.1:8200/v1/sys/health | grep -q '"initialized":'; then
        echo "Vault is responding."
        break
    fi
    echo "Waiting... ($i)"
    sleep 2
done

if [ -s /opt/vault/logs/.vault-token ]; then
    export VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)
elif [ -s /opt/vault/logs/init.json ]; then
    export VAULT_TOKEN=$(jq -r '.root_token' /opt/vault/logs/init.json)
    echo "$VAULT_TOKEN" > /opt/vault/logs/.vault-token
else
    echo "Vault is initialized but token files are missing and unrecoverable."
    vault status
    exit 1
fi

# Determine state
IS_INITIALIZED=$(vault status -format=json | jq -r '.initialized')
IS_SEALED=$(vault status -format=json | jq -r '.sealed')

# -------------------------------
# Only initialize if not already
# -------------------------------
if [ "$IS_INITIALIZED" == "false" ]; then
    echo "Vault is not initialized. Initializing..."

    vault operator init -format=json > /opt/vault/logs/init.json || {
        echo "Vault initialization failed."
        exit 1
    }

    jq -r '.root_token' /opt/vault/logs/init.json > /opt/vault/logs/.vault-token
    jq -r '.unseal_keys_b64[]' /opt/vault/logs/init.json > /opt/vault/logs/unseal-keys.txt

elif [ ! -s /opt/vault/logs/init.json ]; then
    echo "Vault is already initialized but init.json is empty — cannot recover."
    vault status
    exit 1

else
    echo "Vault is already initialized."
fi

ls -la /opt/vault/logs/

# Always try to load the token
if [ -f /opt/vault/logs/.vault-token ]; then
    VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)
elif [ -f /opt/vault/logs/init.json ]; then
    VAULT_TOKEN=$(jq -r '.root_token' /opt/vault/logs/init.json)
    echo "$VAULT_TOKEN" > /opt/vault/logs/.vault-token
else
    echo "Vault is initialized but token files are missing and unrecoverable."
    vault status
    exit 1
fi

export VAULT_TOKEN

# Case 2: Unseal if needed
if [ "$IS_SEALED" == "true" ]; then
    echo "Unsealing Vault..."
    if [ ! -f /opt/vault/logs/unseal-keys.txt ]; then
        echo "Missing unseal-keys.txt. Cannot unseal."
        vault status
        exit 1
    fi
    while read -r key; do
        vault operator unseal "$key"
    done < /opt/vault/logs/unseal-keys.txt
else
    echo "Vault already unsealed."
fi

# Save env file for other containers
cat <<EOF > /opt/vault/logs/vault_env.sh
export VAULT_ADDR=$VAULT_ADDR
export VAULT_API_ADDR=$VAULT_ADDR
export VAULT_TOKEN=$VAULT_TOKEN
EOF

echo "Vault initialized and unsealed."

# Initialise secrets
/usr/local/bin/init_vault.sh

wait $VAULT_PID
