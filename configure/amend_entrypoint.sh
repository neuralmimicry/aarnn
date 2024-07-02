#!/bin/bash

# Script to add a delay to the docker-entrypoint.sh script
# Path to the original entrypoint script
ENTRYPOINT_SCRIPT="/usr/local/bin/docker-entrypoint.sh"

# Temporary file to store the modified script
TEMP_SCRIPT="/tmp/docker-entrypoint.sh"

# Copy the original script to the temporary file
cp $ENTRYPOINT_SCRIPT $TEMP_SCRIPT

# Add a delay after the docker_process_init_files function call
sed -i '/docker_process_init_files \/docker-entrypoint-initdb.d\/*/a\ \ \ \ sleep 5' $TEMP_SCRIPT

# Replace the original script with the modified script
mv $TEMP_SCRIPT $ENTRYPOINT_SCRIPT

# Make the modified script executable
chmod +x $ENTRYPOINT_SCRIPT

echo "docker-entrypoint.sh modified to include a delay after processing init files."
