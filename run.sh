#!/bin/bash

# Ensure the script is being run from the root directory
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Starting the run process..."

# Find all run???.sh files in the specified subfolders
echo "Searching for scripts in $ROOT_DIR/BuildFromGit..."
FILES=$(find "$ROOT_DIR/BuildFromGit" -type f -name 'run[0-9][0-9][0-9].sh')

# Declare an associative array to hold filenames and their numeric indices
declare -A file_map

echo "Processing found scripts..."

# Iterate over each found file
for filepath in $FILES; do
    filename=$(basename "$filepath")
    # Extract the numeric part of the filename
    number="${filename:3:3}"
    # Check if the number is within 000 to 999
    if [[ "$number" =~ ^[0-9]{3}$ ]]; then
        # Remove leading zeros for proper numeric sorting
        number_no_leading_zero=$((10#$number))
        file_map["$number_no_leading_zero"]="$filepath"
    fi
done

# Get the total number of scripts
total_scripts=${#file_map[@]}
echo "Found $total_scripts scripts to run."

# Sort the numbers and execute the scripts in order
sorted_numbers=($(printf '%s\n' "${!file_map[@]}" | sort -n))

# Initialize a counter
counter=1

# Execute the scripts in order
for num in "${sorted_numbers[@]}"; do
    script="${file_map[$num]}"
    echo "Running script $counter of $total_scripts: $script"
    # Ensure the script is executable
    chmod +x "$script"
    # Execute the script
    "$script"
    # Check exit status
    if [ $? -ne 0 ]; then
        echo "Script $script exited with errors."
        exit 1
    else
        echo "Completed script: $script"
    fi
    ((counter++))
done

echo "All scripts completed."
