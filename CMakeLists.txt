cmake_minimum_required(VERSION 3.10)

project(AARNN)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PostgreSQL_ROOT "/usr/pgsql-15")

find_package(PostgreSQL REQUIRED)
find_library(PQXX_LIB pqxx HINTS /usr/lib64)
if(NOT PQXX_LIB)
    message(FATAL_ERROR "pqxx library not found")
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL library not found")
endif()

find_package(VTK REQUIRED)
if(NOT VTK_FOUND)
    message(FATAL_ERROR "VTK library not found")
endif()

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS system filesystem)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost library not found")
endif()

find_package(FFmpeg REQUIRED)
if(NOT FFmpeg_FOUND)
    message(FATAL_ERROR "FFmpeg library not found")
endif()

# Find OpenSSL
find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL library not found")
endif()

# Find OpenMP
find_package(OpenMP REQUIRED)
if(NOT OpenMP_CXX_FOUND)
    message(FATAL_ERROR "OpenMP library not found")
endif()

# Add the src subdirectory to the include paths
include_directories(src)
include_directories(include)
include_directories(${PostgreSQL_INCLUDE_DIR})
include_directories(${VTK_INCLUDE_DIRS})  # Add VTK include directories
include_directories(${Boost_INCLUDE_DIRS})  # Add Boost include directories
include_directories(${OpenSSL_INCLUDE_DIRS})  # Add OpenSSL include directories
include_directories(${FFmpeg_INCLUDE_DIRS})  # Add FFmpeg include directories

link_directories(${PostgreSQL_LIBRARY_DIR})

# Define the target for aarnn.cpp
add_executable(AARNN "src/aarnn.cpp")
target_link_libraries(AARNN PRIVATE Boost::headers Boost::system Boost::filesystem ${PQXX_LIB} ${PostgreSQL_LIBRARIES} ${VTK_LIBRARIES} ${OpenSSL_LIBRARIES} ${FFmpeg_LIBRARIES} OpenMP::OpenMP_CXX)  # Link VTK, Boost, OpenSSL and OpenMP libraries

# Define the target for visualiser.cpp
add_executable(Visualiser "src/visualiser.cpp")
target_link_libraries(Visualiser PRIVATE Boost::headers Boost::system Boost::filesystem ${PQXX_LIB} ${PostgreSQL_LIBRARIES} ${VTK_LIBRARIES} ${OpenSSL_LIBRARIES} ${FFmpeg_LIBRARIES} OpenMP::OpenMP_CXX)  # Link VTK, Boost, OpenSSL and OpenMP libraries
