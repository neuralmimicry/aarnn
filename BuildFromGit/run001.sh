#!/bin/bash

# run001.sh - Run Ansible playbook to install Buildah locally

echo "Starting run001.sh: Running Ansible playbook to install Buildah locally..."

# Check if Ansible is installed
if ! command -v ansible-playbook >/dev/null 2>&1; then
    echo "Ansible is not installed. Please run run000.sh first."
    exit 1
fi

# Define the playbook path
PLAYBOOK_PATH="./BuildFromGit/ansible/install_buildah.yml"

# Check if the playbook exists
if [ ! -f "$PLAYBOOK_PATH" ]; then
    echo "Playbook $PLAYBOOK_PATH not found!"
    exit 1
fi

# Run the Ansible playbook locally with --ask-become-pass
ansible-playbook -i localhost, "$PLAYBOOK_PATH" --connection=local --become --ask-become-pass

# Check the exit status of the Ansible playbook
if [ $? -ne 0 ]; then
    echo "Ansible playbook failed."
    exit 1
else
    echo "Ansible playbook completed successfully."
fi

echo "run001.sh completed successfully."
