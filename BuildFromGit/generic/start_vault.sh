#!/bin/bash
set -e

source /.env

vault server -config=/etc/vault.d/vault.hcl -log-level=info > /opt/vault/logs/vault_output.log 2>&1 &
VAULT_PID=$!

echo "Waiting for Vault to be reachable..."
for i in {1..30}; do
    if curl -s http://127.0.0.1:8200/v1/sys/health | grep -q "initialized"; then
        echo "Vault is responding."
        break
    else
        echo "Waiting... ($i)"
        sleep 2
    fi
done

export VAULT_ADDR=http://127.0.0.1:8200

# Check if Vault is initialized
if vault status | grep -q 'Initialized.*false'; then
    echo "Initializing Vault..."

    vault operator init -format=json > /opt/vault/logs/init.json
    VAULT_TOKEN=$(jq -r '.root_token' /opt/vault/logs/init.json)
    jq -r '.unseal_keys_b64[]' /opt/vault/logs/init.json > /opt/vault/logs/unseal-keys.txt
    echo "$VAULT_TOKEN" > /home/vault/.vault-token
else
    echo "Vault already initialized"
    VAULT_TOKEN=$(cat /home/vault/.vault-token)
fi

# Unseal Vault (assumes default 3 keys required)
if vault status | grep -q 'Sealed.*true'; then
    echo "Unsealing Vault..."
    while read -r key; do
        vault operator unseal "$key"
        sleep 1
    done < /opt/vault/logs/unseal-keys.txt
else
    echo "Vault already unsealed"
fi

export VAULT_TOKEN
echo "Vault root token: $VAULT_TOKEN"

# Save for future containers
cat <<EOF > /opt/vault/logs/vault_env.sh
export VAULT_ADDR=$VAULT_ADDR
export VAULT_TOKEN=$VAULT_TOKEN
EOF

# Seed secrets
/usr/local/bin/init_vault.sh

wait $VAULT_PID
