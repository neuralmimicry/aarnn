#!/bin/bash
set -e

# 1) Check required env vars
check_variables() {
  required_vars=(
    POSTGRES_DB POSTGRES_HOST POSTGRES_PORT
    PULSE_SERVER PULSE_COOKIE PULSE_SINK PULSE_SOURCE
    VAULT_ADDR VAULT_API_ADDR VAULT_TOKEN
  )
  for v in "${required_vars[@]}"; do
    val=$(echo "${!v}" | sed 's/^"//; s/"$//')
    [[ -z "$val" ]] && echo "Error: $v is not set." && exit 1
    export "$v"="$val"
  done
}
check_variables

# 2) Ensure background Node processes are cleaned up on exit
cleanup() {
  echo "Shutting down Node processes…"
  kill "${CLIENT_PID}" "${SERVER_PID}" 2>/dev/null || true
}
trap cleanup EXIT

# 3) Launch your Node.js pieces in the background
echo "▶ Starting Node client-server…"
node /app/webviewer/clientserver.js & CLIENT_PID=$!

echo "▶ Starting Node visualiser server…"
node /app/webviewer/server/visualiser.js & SERVER_PID=$!

# 4) (Optional) Dump Vault info for debug
echo "VAULT_ADDR: $VAULT_ADDR"
echo "VAULT_API_ADDR: $VAULT_API_ADDR"
echo "VAULT_TOKEN: $VAULT_TOKEN"

# 5) Finally boot the C++ Visualiser (will block until it exits)
echo "▶ Launching C++ Visualiser…"
/app/Visualiser
