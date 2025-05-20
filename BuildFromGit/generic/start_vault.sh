#!/bin/bash
set -e

source /.env
export VAULT_ADDR=http://127.0.0.1:8200

vault server -config=/etc/vault.d/vault.hcl -log-level=info > /opt/vault/logs/vault_output.log 2>&1 &
VAULT_PID=$!

echo "Waiting for Vault to be reachable..."
for i in {1..30}; do
    if curl -s http://127.0.0.1:8200/v1/sys/health | grep -q "initialized"; then
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
    VAULT_TOKEN=$(jq -r '.root_token' /opt/vault/logs/init.json)
    jq -r '.unseal_keys_b64[]' /opt/vault/logs/init.json > /opt/vault/logs/unseal-keys.txt

    echo "$VAULT_TOKEN" > /opt/vault/logs/.vault-token
else
    echo "Vault already initialized"

    if [ -f /opt/vault/logs/.vault-token ]; then
        VAULT_TOKEN=$(cat /opt/vault/logs/.vault-token)
    elif [ -f /opt/vault/logs/init.json ]; then
        VAULT_TOKEN=$(jq -r '.root_token' /opt/vault/logs/init.json)
        echo "$VAULT_TOKEN" > /opt/vault/logs/.vault-token
    else
        echo "Vault token file and init.json are both missing! Cannot proceed."
        exit 1
    fi
fi

# Unseal Vault
if vault status | grep -q 'Sealed.*true'; then
    echo "Unsealing Vault..."
    while read -r key; do
        vault operator unseal "$key"
        sleep 1
    done < /opt/vault/logs/unseal-keys.txt
else
    echo "Vault already unsealed."
fi

echo "Vault token: $VAULT_TOKEN"
export VAULT_TOKEN

# Save for other containers
cat <<EOF > /opt/vault/logs/vault_env.sh
export VAULT_ADDR=$VAULT_ADDR
export VAULT_TOKEN=$VAULT_TOKEN
export VAULT_API_ADDR=$VAULT_API_ADDR
EOF

# Seed the secret store
/usr/local/bin/init_vault.sh

wait $VAULT_PID
