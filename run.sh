#!/bin/bash

# POSTGRES_RUNNING=$(docker ps x| grep aarnn-postgres | xargs)

# if [ "${POSTGRES_RUNNING}" != "" ] ; then
#     docker run --name postgres_container -e POSTGRES_PASSWORD=GeneTics99! -d -p 5432:5432 postgres:latest
# fi

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

#  make -j $(nprocs)

cp ../db_connection.conf . 

./AARNN
./Visualiser


# # Run valgrind to check memory stuff
# valgrind --tool=memcheck ./executables/Pointers