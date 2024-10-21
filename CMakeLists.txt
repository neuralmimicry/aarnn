cmake_minimum_required(VERSION 3.20)

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

# Add AddressSanitizer flags
#set(SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer -fsanitize=leak -fsanitize=undefined")
set(SANITIZER_FLAGS "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CGAL_DIR "/usr/lib/x86_64-linux-gnu/cmake/CGAL/")
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

# Find FFmpeg using pkg-config
find_package(PkgConfig REQUIRED)

pkg_check_modules(FFmpeg REQUIRED libavcodec libavformat libavutil libswscale libswresample)

if(NOT FFmpeg_FOUND)
    message(FATAL_ERROR "FFmpeg libraries not found")
endif()

# Include directories
include_directories(${FFmpeg_INCLUDE_DIRS})

# Libraries
set(FFmpeg_LIBRARIES ${FFmpeg_LIBRARIES})

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

# Find CURL
find_package(CURL REQUIRED)
if(NOT CURL_FOUND)
    message(FATAL_ERROR "CURL library not found")
endif()

# Find CGAL
find_package(CGAL REQUIRED)
if(NOT CGAL_FOUND)
    message(FATAL_ERROR "CGAL library not found")
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

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# Add the src subdirectory to the include paths
include_directories(include)
include_directories(src)
include_directories(thirdparty/websocketpp)
include_directories(${PostgreSQL_INCLUDE_DIR})
include_directories(${VTK_INCLUDE_DIRS})  # Add VTK include directories
include_directories(${Boost_INCLUDE_DIRS})  # Add Boost include directories
include_directories(${OpenSSL_INCLUDE_DIRS})  # Add OpenSSL include directories
include_directories(${FFmpeg_INCLUDE_DIRS})  # Add FFmpeg include directories
include_directories(${PORTAUDIO_INCLUDE_DIRS}) # Add PortAudio include directories
include_directories(${PulseAudio_INCLUDE_DIRS}) # Add PulseAudio include directories
include_directories(${FFTW3_INCLUDE_DIRS}) # Add FFTW3 include directories
include_directories(${CURL_INCLUDE_DIRS}) # Add CURL include directories
include_directories(${CGAL_INCLUDE_DIRS}) # Add CGAL include directories

link_directories(${PostgreSQL_LIBRARY_DIR})
link_directories(${PORTAUDIO_LIBRARY_DIR})
link_directories(${PulseAudio_LIBRARY_DIRS})
link_directories(${FFTW3_LIBRARY_DIRS})
link_directories(${LOCAL_LIB64})

add_executable(AARNN ${AARNN_SRC}
        include/vclient.h
        src/shared_libs/vclient.cpp
        include/Neuron.h
        include/utils.h
        include/abstractObject.h
        include/avTimerCallback.h
        include/Axon.h
        include/AxonBouton.h
        include/AxonBranch.h
        include/AxonHillock.h
        include/BodyComponent.h
        include/BodyShapeComponent.h
        include/boostincludes.h
        include/Dendrite.h
        include/DendriteBouton.h
        include/DendriteBranch.h
        include/Effector.h
        include/Logger.h
        include/msock.h
        include/NeuronParameters.h
        include/nvTimerCallback.h
        include/portaudioMicThread.h
        include/Position.h
        include/PulseAudioMic.h
        include/SensoryReceptor.h
        include/Shape3D.h
        include/Soma.h
        include/SynapticGap.h
        include/ThreadSafeQueue.h
        include/vtkincludes.h
        src/aarnn/config.cpp
        include/config.h
        include/database.h
        include/audio.h
        include/timer.h
        src/aarnn/database.cpp
        src/aarnn/audio.cpp
        src/shared_libs/Logger.cpp
        src/aarnn/timer.cpp
        src/aarnn/main.cpp
        src/aarnn/utils.cpp
        include/globals.h)
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
        ${CURL_LIBRARIES}
        ${CGAL_LIBRARIES}
)

# Copy simulation.conf into the same directory as the AARNN executable
add_custom_command(TARGET AARNN POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/configure/simulation.conf"
        "$<TARGET_FILE_DIR:AARNN>/simulation.conf"
)

add_executable(Visualiser ${VISUALISER_SRC}
        include/vclient.h
        include/Logger.h
        src/shared_libs/Logger.cpp
        src/shared_libs/vclient.cpp
        src/visualiser/wss.cpp)
target_link_libraries(Visualiser PRIVATE
        ${VTK_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OpenSSL_LIBRARIES}
        ${FFTW3_LIBRARIES}
        ${PQXX_LIB}
        ${CURL_LIBRARIES}
        ${CGAL_LIBRARIES}
        nlohmann_json::nlohmann_json
)

# Copy simulation.conf into the same directory as the AARNN executable
add_custom_command(TARGET Visualiser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/configure/db_connection.conf"
        "$<TARGET_FILE_DIR:Visualiser>/db_connection.conf"
)
