#!/bin/bash
set -e

export VAULT_ADDR=http://127.0.0.1:8200

vault server -config=/etc/vault.d/vault.hcl -log-level=info > /opt/vault/logs/vault_output.log 2>&1 &
VAULT_PID=$!

# Wait until Vault is responding
echo "Waiting for Vault to be reachable..."
for i in {1..30}; do
    if curl -s http://127.0.0.1:8200/v1/sys/health | grep -q '"initialized":'; then
        echo "Vault is responding."
        break
    fi
    echo "Waiting... ($i)"
    sleep 2
done

# Check if Vault is initialized
if vault status | grep -q 'Initialized.*false'; then
    echo "Vault is not initialized. Initializing..."
    vault operator init -format=json > /opt/vault/logs/init.json
    jq -r '.root_token' /opt/vault/logs/init.json > /opt/vault/logs/.vault-token
    jq -r '.unseal_keys_b64[]' /opt/vault/logs/init.json > /opt/vault/logs/unseal-keys.txt
else
    echo "Vault already initialized"
    # Recover from missing .vault-token if possible
    if [ -f /opt/vault/logs/.vault-token ]; then
        echo "Vault token file exists."
    elif [ -f /opt/vault/logs/init.json ]; then
        echo "Vault token missing, recovering from init.json..."
        jq -r '.root_token' /opt/vault/logs/init.json > /opt/vault/logs/.vault-token
    else
        echo "Vault token file and init.json are both missing! Cannot proceed."
        exit 1
    fi
fi

VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)

# Unseal vault if necessary
if vault status | grep -q 'Sealed.*true'; then
    echo "Unsealing Vault..."
    while read -r key; do
        vault operator unseal "$key"
    done < /opt/vault/logs/unseal-keys.txt
else
    echo "Vault already unsealed."
fi

export VAULT_TOKEN
echo "Vault initialized and unsealed."

# Save token info
cat <<EOF > /opt/vault/logs/vault_env.sh
export VAULT_ADDR=$VAULT_ADDR
export VAULT_API_ADDR=$VAULT_ADDR
export VAULT_TOKEN=$VAULT_TOKEN
EOF

# Initialise secrets
/usr/local/bin/init_vault.sh

wait $VAULT_PID
