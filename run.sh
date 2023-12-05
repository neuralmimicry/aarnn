#!/bin/bash

# Remove existing build directory
rm -r build

# Remove the AARNN executable file (if it exists)
rm -f AARNN

# Create a new build directory and navigate into it
mkdir build && cd build

# Run CMake from within the build directory
cmake ..

# Build the project
cmake --build .

# # Run the Pointers executable (if it was built successfully)
# if [ -f "../executables/AARNN" ]; then
#     cd ..
#     ./executables/Pointers
# fi

# # Run valgrind to check memory stuff
# valgrind --tool=memcheck ./executables/Pointers