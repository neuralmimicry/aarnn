#!/bin/bash

# run000.sh - Check for Ansible installation and install if missing

echo "Starting run000.sh: Checking for Ansible installation..."

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install Ansible on Debian/Ubuntu
install_ansible_debian() {
    echo "Installing Ansible on Debian/Ubuntu..."
    sudo apt-get update -y
    sudo apt-get install -y software-properties-common
    sudo apt-add-repository --yes --update ppa:ansible/ansible
    sudo apt-get install -y ansible
}

# Function to install Ansible on RHEL/CentOS
install_ansible_centos() {
    echo "Installing Ansible on RHEL/CentOS..."
    sudo yum install -y epel-release
    sudo yum install -y ansible
}

# Function to install Ansible on Fedora
install_ansible_fedora() {
    echo "Installing Ansible on Fedora..."
    sudo dnf install -y ansible
}

# Function to install Ansible on macOS using Homebrew
install_ansible_macos() {
    echo "Installing Ansible on macOS..."
    brew install ansible
}

# Detect the operating system
OS="$(uname -s)"
if command_exists ansible; then
    echo "Ansible is already installed."
else
    echo "Ansible is not installed. Proceeding with installation..."
    if [ "$OS" = "Linux" ]; then
        # Determine the specific Linux distribution
        if [ -f /etc/debian_version ]; then
            # Debian/Ubuntu
            install_ansible_debian
        elif [ -f /etc/redhat-release ]; then
            # RHEL/CentOS
            install_ansible_centos
        elif [ -f /etc/fedora-release ]; then
            # Fedora
            install_ansible_fedora
        else
            echo "Unsupported Linux distribution. Please install Ansible manually."
            exit 1
        fi
    elif [ "$OS" = "Darwin" ]; then
        # macOS
        if command_exists brew; then
            install_ansible_macos
        else
            echo "Homebrew is not installed. Installing Homebrew..."
            /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            # Add Homebrew to PATH
            eval "$(/opt/homebrew/bin/brew shellenv)" 2>/dev/null || eval "$(/usr/local/bin/brew shellenv)"
            install_ansible_macos
        fi
    else
        echo "Unsupported operating system: $OS. Please install Ansible manually."
        exit 1
    fi

    # Verify installation
    if command_exists ansible; then
        echo "Ansible has been installed successfully."
    else
        echo "Ansible installation failed. Please check for errors."
        exit 1
    fi
fi

echo "run000.sh completed successfully."
