#!/bin/bash

# Purpose: Setup PulseAudio environment variables and ensure PulseAudio is running correctly

# Define the path to the .env file
ENV_FILE="./.env"

# Function to update the .env file
update_env_file() {
    local VAR_NAME="$1"
    local VAR_VALUE="$2"

    # Escape special characters in the value
    VAR_VALUE="${VAR_VALUE//\\/\\\\}"
    VAR_VALUE="${VAR_VALUE//\"/\\\"}"

    # If variable exists in .env, replace it; else, append it
    if grep -q "^${VAR_NAME}=" "$ENV_FILE"; then
        # Replace the line
        sed -i "s|^${VAR_NAME}=.*|${VAR_NAME}=${VAR_VALUE}|g" "$ENV_FILE"
    else
        # Append the variable
        echo "${VAR_NAME}=${VAR_VALUE}" >> "$ENV_FILE"
    fi
}

# Check if XDG_RUNTIME_DIR is set, if not, set it to a default value
if [ -z "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR="/run/user/$(id -u)"
    echo "XDG_RUNTIME_DIR set to $XDG_RUNTIME_DIR"
fi

# Ensure the runtime directory exists
if [ ! -d "$XDG_RUNTIME_DIR" ]; then
    echo "Creating XDG_RUNTIME_DIR at $XDG_RUNTIME_DIR"
    mkdir -p "$XDG_RUNTIME_DIR"
    chmod 700 "$XDG_RUNTIME_DIR"
fi

# Function to restart PulseAudio if auto_null is detected
restart_pulseaudio() {
    local attempts=0
    local max_attempts=2

    while [ $attempts -lt $max_attempts ]; do
        # Check PulseAudio status
        local info=$(pactl info)
        echo "$info"
        if ! echo "$info" | grep -q "auto_null"; then
            echo "PulseAudio started successfully with valid sink and source."
            return 0
        fi

        # Increment attempts and restart PulseAudio
        attempts=$((attempts + 1))
        echo "PulseAudio has auto_null for sink/source. Restarting PulseAudio (attempt $attempts)..."
        # Kill any existing PulseAudio processes
        pulseaudio --kill
        pkill -9 pulseaudio
        rm -rf /run/user/"$(id -u)"/pulse
        sleep 10
    done

    echo "Failed to start PulseAudio with valid sink and source after $max_attempts attempts."
    return 1
}

# Restart PulseAudio if necessary
if ! restart_pulseaudio; then
    exit 1
fi

# Detect the PulseAudio socket
if [ -S "$XDG_RUNTIME_DIR/pulse/native" ]; then
    export PULSE_SERVER="unix:$XDG_RUNTIME_DIR/pulse/native"
    echo "PULSE_SERVER set to $PULSE_SERVER"
    update_env_file "PULSE_SERVER" "$PULSE_SERVER"
else
    echo "PulseAudio socket not found at $XDG_RUNTIME_DIR/pulse/native"
    exit 1
fi

# Clean up the PulseAudio sinks and sources output
clean_pactl_output() {
    echo "$1" | awk '{print $2}' | grep 'alsa_'
}

# Get the PulseAudio sinks and sources
SINKS=$(clean_pactl_output "$(pactl list short sinks)")
SOURCES=$(clean_pactl_output "$(pactl list short sources)")

# Check if $PULSE_SINK exists in the sinks list
if [[ $SINKS =~ $PULSE_SINK ]] && [[ $PULSE_SINK != "" ]]; then
    echo "$PULSE_SINK found in sinks list."
else
    if [[ $SINKS == "" ]]; then
        echo "Sinks list is empty, leaving PULSE_SINK unchanged."
    else
        # Get the first sink from the list
        FIRST_SINK=$(echo "$SINKS" | head -n1)
        PULSE_SINK=${FIRST_SINK%% *}
        echo "$PULSE_SINK not found in sinks list. Setting PULSE_SINK to first sink: $PULSE_SINK"
    fi
fi

# Check if $PULSE_SOURCE exists in the sources list
if [[ $SOURCES =~ $PULSE_SOURCE ]] && [[ $PULSE_SOURCE != "" ]]; then
    echo "$PULSE_SOURCE found in sources list."
else
    if [[ $SOURCES == "" ]]; then
        echo "Sources list is empty, leaving PULSE_SOURCE unchanged."
    else
        # Get the first source from the list
        FIRST_SOURCE=$(echo "$SOURCES" | head -n1)
        PULSE_SOURCE=${FIRST_SOURCE%% *}
        echo "$PULSE_SOURCE not found in sources list. Setting PULSE_SOURCE to first source: $PULSE_SOURCE"
    fi
fi

# Export and update PULSE_SINK
export PULSE_SINK
update_env_file "PULSE_SINK" "$PULSE_SINK"

# Export and update PULSE_SOURCE
export PULSE_SOURCE
update_env_file "PULSE_SOURCE" "$PULSE_SOURCE"

export PULSE_COOKIE=/root/.config/pulse/cookie
update_env_file "PULSE_COOKIE" "$PULSE_COOKIE"

echo "PulseAudio environment setup complete."
