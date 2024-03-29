cmake_minimum_required(VERSION 3.10)

project(AARNN)

# Set the build type to Debug to include debugging symbols
set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)

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

# Find PortAudio
find_package(PortAudio REQUIRED)
if(NOT PortAudio_FOUND)
    message(FATAL_ERROR "PortAudio library not found")
endif()
message(STATUS "PortAudio Include Dirs: ${PORTAUDIO_INCLUDE_DIRS}")
message(STATUS "PortAudio Library Dirs: ${PORTAUDIO_LIBRARY_DIRS}")
message(STATUS "PortAudio Libraries: ${PORTAUDIO_LIBRARIES}")
message(STATUS "PortAudio CFlags: ${PORTAUDIO_CFLAGS}")
if(NOT PORTAUDIO_INCLUDE_DIRS)
    set(PORTAUDIO_INCLUDE_DIRS "/usr/local/include")
    message(STATUS "Using default PortAudio include dir: ${PORTAUDIO_INCLUDE_DIRS}")
endif()
if(NOT PORTAUDIO_LIBRARY_DIRS)
    set(PORTAUDIO_LIBRARY_DIRS "/usr/local/lib")
    message(STATUS "Using default PortAudio library dir: ${PORTAUDIO_LIBRARY_DIRS}")
endif()
if(NOT PORTAUDIO_LIBRARIES)
    set(PORTAUDIO_LIBRARIES "-pthread -lportaudio -lm -lpthread -lasound")
    message(STATUS "Using default PortAudio libraries: ${PORTAUDIO_LIBRARIES}")
endif()

# Find PulseAudio
find_package(PulseAudio REQUIRED)
if(NOT PulseAudio_FOUND)
    message(FATAL_ERROR "PulseAudio library not found")
endif()
message(STATUS "PulseAudio Include Dirs: ${PulseAudio_INCLUDE_DIRS}")
message(STATUS "PulseAudio Library Dirs: ${PulseAudio_LIBRARY_DIRS}")
message(STATUS "PulseAudio Libraries: ${PulseAudio_LIBRARIES}")
message(STATUS "PulseAudio CFlags: ${PulseAudio_CFLAGS}")
if(NOT PulseAudio_INCLUDE_DIRS)
    set(PulseAudio_INCLUDE_DIRS "/usr/local/include")
    message(STATUS "Using default PulseAudio include dir: ${PulseAudio_INCLUDE_DIRS}")
endif()
if(NOT PulseAudio_LIBRARY_DIRS)
    set(PulseAudio_LIBRARY_DIRS "/usr/local/lib")
    message(STATUS "Using default PulseAudio library dir: ${PulseAudio_LIBRARY_DIRS}")
endif()
if(NOT PulseAudio_LIBRARIES)
    set(PulseAudio_LIBRARIES "-lpulse -lpulse-simple")
    message(STATUS "Using default PulseAudio libraries: ${PulseAudio_LIBRARIES}")
endif()
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
include_directories(src)
include_directories(include)
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
link_directories(/usr/local/lib64)

# Define the target for aarnn.cpp
add_executable(AARNN "src/aarnn.cpp")
target_link_libraries(AARNN PRIVATE Boost::headers Boost::system Boost::filesystem ${PQXX_LIB} ${PostgreSQL_LIBRARIES} ${VTK_LIBRARIES} ${OpenSSL_LIBRARIES} ${FFmpeg_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${PulseAudio_LIBRARIES} ${FFTW3_LIBRARIES} OpenMP::OpenMP_CXX)  # Link VTK, Boost, OpenSSL, PortAudio, PulseAudio and OpenMP libraries


# Define the target for visualiser.cpp
add_executable(Visualiser "src/visualiser.cpp")
target_link_libraries(Visualiser PRIVATE Boost::headers Boost::system Boost::filesystem ${PQXX_LIB} ${PostgreSQL_LIBRARIES} ${VTK_LIBRARIES} ${OpenSSL_LIBRARIES} ${FFmpeg_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${PulseAudio_LIBRARIES} ${FFTW3_LIBRARIES} OpenMP::OpenMP_CXX)  # Link VTK, Boost, OpenSSL, PortAudio, PulseAudio and OpenMP libraries
