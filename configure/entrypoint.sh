#!/bin/bash
set -e
source /app/.env

required_vars=("POSTGRES_USERNAME" "POSTGRES_PASSWORD" "POSTGRES_DBNAME" "POSTGRES_PORT_EXPOSE" "PGDATA")
for var in "${required_vars[@]}"; do
  # Remove quotes from the variable value
  value=$(echo "${!var}" | sed 's/^"//; s/"$//')
  export "$var"="$value"

  if [ -z "$value" ]; then
    echo "Error: Environment variable $var is not set."
    exit 1
  fi
done

# Pause for initial startup
sleep 10

# Function to configure PostgreSQL
configure_postgres() {
  PG_HBA_CONF="$PGDATA/pg_hba.conf"
  POSTGRESQL_CONF="$PGDATA/postgresql.conf"

  # Update pg_hba.conf
  sed -i "s/^local\s*all\s*all\s*peer/local all all trust/" "$PG_HBA_CONF"
  echo "host all all 0.0.0.0/0 md5" >> "$PG_HBA_CONF"

  # Update postgresql.conf
  sed -i "s/#listen_addresses = 'localhost'/listen_addresses = '*'/" "$POSTGRESQL_CONF"
  sed -i "s/#port = 5432/port = $POSTGRES_PORT_EXPOSE/" "$POSTGRESQL_CONF"
  sed -i "s/port = 5432/port = $POSTGRES_PORT_EXPOSE/" "$POSTGRESQL_CONF"
  sed -i "s/port = 5433/port = $POSTGRES_PORT_EXPOSE/" "$POSTGRESQL_CONF"
  sed -i "s/port = 5434/port = $POSTGRES_PORT_EXPOSE/" "$POSTGRESQL_CONF"
}

# Initialize the database if not already initialized
if [ ! -s "$PGDATA/PG_VERSION" ]; then
  # Run the original entrypoint script to initialize the database
  docker-entrypoint.sh postgres &

  # Wait for PostgreSQL to be ready
  for i in {1..60}; do
    if su postgres -c "pg_isready -U ${POSTGRES_USERNAME}" > /dev/null 2>&1; then
      echo "PostgreSQL is ready."
      break
    else
      echo "Waiting for PostgreSQL to be ready... ($i)"
      sleep 2
    fi

    if [ "$i" -eq 60 ]; then
      echo "PostgreSQL did not become ready in time."
      exit 1
    fi
  done

  # Enable initial startup to complete uninterrupted (init.sql to run, otherwise no neurons db gets created)
  sleep 10

  # Configure PostgreSQL for remote access
  configure_postgres

  # Stop the server after initial configuration
  su postgres -c "pg_ctl -D \"$PGDATA\" -m fast -w stop"
fi

# --- Begin Firewall Configuration ---

# Install iptables if not installed
if ! command -v iptables >/dev/null 2>&1; then
  echo "Installing iptables..."
  apt-get update
  apt-get install -y iptables sudo
fi

# Allow incoming traffic on $POSTGRES_PORT_EXPOSE
echo "Configuring firewall to allow port $POSTGRES_PORT_EXPOSE..."
sudo iptables -I INPUT -p tcp --dport "$POSTGRES_PORT_EXPOSE" -j ACCEPT

# --- End Firewall Configuration ---

# Call the original entrypoint script of the postgres image to start the server
exec docker-entrypoint.sh postgres
