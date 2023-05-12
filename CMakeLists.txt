cmake_minimum_required(VERSION 3.10)

project(AARNN)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PostgreSQL_ROOT "/usr/pgsql-15")

find_package(PostgreSQL REQUIRED)
find_library(PQXX_LIB pqxx HINTS /usr/lib64)
if(NOT PQXX_LIB)
    message(FATAL_ERROR "pqxx library not found")
endif()

# Add the src subdirectory to the include paths
include_directories(src)
include_directories(include)
include_directories (${PostgreSQL_INCLUDE_DIR})
link_directories(${PostgreSQL_LIBRARY_DIR})

# Add all source files in the src subfolder
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add the -pthread option for GCC
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_executable(AARNN ${SOURCES})
target_link_libraries(AARNN PRIVATE ${PQXX_LIB} ${PostgreSQL_LIBRARIES})
