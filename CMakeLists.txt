cmake_minimum_required(VERSION 3.10)

if(CMAKE_BUILD_TYPE STREQUAL "")
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
message("BUILD TYPE " ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Debug Release RelWithDebInfo MinSizeRel Coverage")
set(DISPLAY_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage" CACHE STRING "Configs" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})

execute_process(
        COMMAND bash -c "printf \"%s\" $(cat /etc/os-release | grep -E \"^ID=[^\\n]*|^VERSION_ID=[^\\n]*\" | cut -f2 -d= | tr -d '\"')"
        OUTPUT_VARIABLE LINUX_VERSION
)
message("Linux version is '${LINUX_VERSION}'")

set(COVERAGE_COMPILER_FLAGS "-g -O0 -fno-default-inline --coverage -fprofile-abs-path -fprofile-arcs -fno-inline -fno-inline-small-functions -ftest-coverage -lgcov" CACHE INTERNAL "")

set(CMAKE_CXX_FLAGS "-rdynamic -Wall -Werror -DBOOST_ALLOW_DEPRECATED_HEADERS -DBOOST_BIND_GLOBAL_PLACEHOLDERS" CACHE STRING "Default C++ compiler flags")
set(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE STRING "g++ debug flags")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2" CACHE STRING "g++ rel+gbg flags")
set(CMAKE_CXX_FLAGS_COVERAGE "${COVERAGE_COMPILER_FLAGS}" CACHE STRING "g++ coverage flags." FORCE )
set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "g++ release flags")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Boost_USE_STATIC_LIBS=${Boost_USE_STATIC_LIBS}")

#
# The Project
#
project(AARNN)

file(GLOB AARNN_SRC
    "src/*.cpp"
)

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

message("Boost_USE_STATIC_LIBS=${Boost_USE_STATIC_LIBS}")
if(NOT Boost_USE_STATIC_LIBS)
    set(Boost_USE_STATIC_LIBS OFF)
endif()
message("Boost_USE_STATIC_LIBS=${Boost_USE_STATIC_LIBS}")
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
set(PULSEAUDIO_LIBRARIES pulse pulse-simple)

# Find FFTW
#find_package(FFTW3 REQUIRED)
#if(NOT FFTW3_FOUND)
#    message(FATAL_ERROR "FFTW3 library not found")
#endif()

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
include_directories(include AFTER SYSTEM
    ${PostgreSQL_INCLUDE_DIR} AFTER SYSTEM
    ${VTK_INCLUDE_DIRS} AFTER SYSTEM
    ${Boost_INCLUDE_DIRS} AFTER SYSTEM
    ${OpenSSL_INCLUDE_DIRS} AFTER SYSTEM
    ${FFmpeg_INCLUDE_DIRS} AFTER SYSTEM
    ${PORTAUDIO_INCLUDE_DIRS} AFTER SYSTEM
    ${PulseAudio_INCLUDE_DIRS} AFTER SYSTEM
    ${FFTW3_INCLUDE_DIRS})

link_directories(${PostgreSQL_LIBRARY_DIR}
    ${PORTAUDIO_LIBRARY_DIR}
    ${PulseAudio_LIBRARY_DIRS}
    ${FFTW3_LIBRARY_DIRS}
    ${LOCAL_LIB64})

add_subdirectory(src)
