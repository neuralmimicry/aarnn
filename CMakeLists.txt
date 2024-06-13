cmake_minimum_required(VERSION 3.10)

project(AARNN)

file(GLOB AARNN_SRC
    "src/aarnn/*.cpp"
)
file(GLOB VISUALISER_SRC
        "src/visualiser/*.cpp"
)

# Set the build type to Debug to include debugging symbols
set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PostgreSQL_ROOT "/usr/include/postgresql/14/server")

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

message(STATUS "VTK INCLUDE : ${VTK_DIR}")
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

# Use pkg-config to get the necessary flags
execute_process(
    COMMAND pkg-config --cflags --libs portaudio-2.0
    OUTPUT_VARIABLE PORTAUDIO_PKG_CONFIG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Set PortAudio_LIBRARIES and PORTAUDIO_INCLUDE_DIRS using the pkg-config output
string(REGEX MATCHALL "-I([^ \t\n]*)"
    PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_PKG_CONFIG})

set(PORTAUDIO_LIBRARIES ${PORTAUDIO_PKG_CONFIG})
message(STATUS "PortAudio Libraries: ${PORTAUDIO_LIBRARIES}")

if(NOT PORTAUDIO_LIBRARIES)
    message(FATAL_ERROR "PortAudio library not found")
endif()

message(STATUS "PortAudio Include Dirs: ${PORTAUDIO_INCLUDE_DIRS}")

# PulseAudio directories
set(PULSEAUDIO_INCLUDE_DIRS "/usr/include/pulseaudio")
set(PULSEAUDIO_LIBRARY_DIRS "/usr/lib/x86_64-linux-gnu")

# PulseAudio libraries
#set(PULSEAUDIO_LIBRARIES "-lpulse -lpulse-simple")
set(PULSEAUDIO_LIBRARIES pulse pulse-simple)

message(STATUS "PulseAudio Include Dirs: ${PULSEAUDIO_INCLUDE_DIRS}")
message(STATUS "PulseAudio Library Dirs: ${PULSEAUDIO_LIBRARY_DIRS}")
message(STATUS "PulseAudio Libraries: ${PULSEAUDIO_LIBRARIES}")

# Find FFTW
#find_package(FFTW3 REQUIRED)
#if(NOT FFTW3_FOUND)
#    message(FATAL_ERROR "FFTW3 library not found")
#endif()
message(STATUS "FFTW3 Include Dirs: ${FFTW3_INCLUDE_DIRS}")
message(STATUS "FFTW3 Library Dirs: ${FFTW3_LIBRARY_DIRS}")
message(STATUS "FFTW3 Libraries: ${FFTW3_LIBRARIES}")
if(NOT FFTW3_INCLUDE_DIRS)
    set(FFTW3_INCLUDE_DIRS "/usr/local/include")
    message(STATUS "Using default FFTW3 include dir: ${FFTW3_INCLUDE_DIRS}")
endif()
if(NOT FFTW3_LIBRARY_DIRS)
    set(FFTW3_LIBRARY_DIRS "/usr/local/lib")
    message(STATUS "Using default FFTW3 library dir: ${FFTW3_LIBRARY_DIRS}")
endif()
if(NOT FFTW3_LIBRARIES)
    set(FFTW3_LIBRARIES "-lfftw3")
    message(STATUS "Using default FFTW3 libraries: ${FFTW3_LIBRARIES}")
endif()

# Add the src subdirectory to the include paths
include_directories(include)
include_directories(src)
include_directories(${PostgreSQL_INCLUDE_DIR})
include_directories(${VTK_INCLUDE_DIRS})  # Add VTK include directories
include_directories(${Boost_INCLUDE_DIRS})  # Add Boost include directories
include_directories(${OpenSSL_INCLUDE_DIRS})  # Add OpenSSL include directories
include_directories(${FFmpeg_INCLUDE_DIRS})  # Add FFmpeg include directories
include_directories(${PORTAUDIO_INCLUDE_DIRS}) # Add PortAudio include directories
include_directories(${PulseAudio_INCLUDE_DIRS}) # Add PulseAudio include directories
include_directories(${FFTW3_INCLUDE_DIRS}) # Add FFTW3 include directories

link_directories(${PostgreSQL_LIBRARY_DIR})
link_directories(${PORTAUDIO_LIBRARY_DIR})
link_directories(${PulseAudio_LIBRARY_DIRS})
link_directories(${FFTW3_LIBRARY_DIRS})
link_directories(${LOCAL_LIB64})

add_executable(AARNN ${AARNN_SRC})
target_link_libraries(AARNN PRIVATE
        ${VTK_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OpenSSL_LIBRARIES}
        ${FFmpeg_LIBRARIES}
        ${PORTAUDIO_LIBRARIES}
        ${PULSEAUDIO_LIBRARIES}
        ${FFTW3_LIBRARIES}
        ${PQXX_LIB}
)
add_executable(Visualiser ${VISUALISER_SRC})
target_link_libraries(Visualiser PRIVATE
        ${VTK_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OpenSSL_LIBRARIES}
        ${FFTW3_LIBRARIES}
        ${PQXX_LIB}
)
