cmake_minimum_required(VERSION 3.20)

project(AARNN)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Release, Debug)" FORCE)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include custom CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Set compilation flags
set(SANITIZER_FLAGS "")  # Uncomment for debugging: "-fsanitize=address -fno-omit-frame-pointer"
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

# Define source files
file(GLOB AARNN_SRC "src/aarnn/*.cpp")
file(GLOB VISUALISER_SRC "src/visualiser/*.cpp")

# Add project includes
set(INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/thirdparty/websocketpp
)
include_directories(${INCLUDE_DIRS})

# Fetch third-party dependencies
include(FetchContent)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(json)

# Find required libraries
find_package(PostgreSQL REQUIRED)
find_library(PQXX_LIB pqxx HINTS /usr/lib64)
if(NOT PQXX_LIB)
    message(FATAL_ERROR "pqxx library not found")
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(VTK REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(OpenSSL REQUIRED)
find_package(OpenMP REQUIRED)
find_package(CURL REQUIRED)
find_package(CGAL REQUIRED)

if(NOT FFTW3_FOUND)
    set(FFTW3_INCLUDE_DIRS "/usr/local/include")
    set(FFTW3_LIBRARY_DIRS "/usr/local/lib")
    set(FFTW3_LIBRARIES "-lfftw3")
endif()

# Find FFmpeg using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFmpeg REQUIRED libavcodec libavformat libavutil libswscale libswresample)

# Find PortAudio
execute_process(COMMAND pkg-config --cflags --libs portaudio-2.0
        OUTPUT_VARIABLE PORTAUDIO_PKG_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCHALL "-I([^ \t\n]*)" PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_PKG_CONFIG})
set(PORTAUDIO_LIBRARIES ${PORTAUDIO_PKG_CONFIG})
if(NOT PORTAUDIO_LIBRARIES)
    message(FATAL_ERROR "PortAudio library not found")
endif()

# Define executable AARNN
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
        include/NeuronalComponent.h
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
        include/globals.h
)

# Link dependencies for AARNN
target_link_libraries(AARNN PRIVATE
        ${VTK_LIBRARIES}
        ${PostgreSQL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OpenSSL_LIBRARIES}
        ${FFmpeg_LIBRARIES}
        ${PORTAUDIO_LIBRARIES}
        ${OpenMP_CXX_FLAGS}
        ${PQXX_LIB}
        ${CURL_LIBRARIES}
        ${CGAL_LIBRARIES}
)

# Define executable Visualiser
add_executable(Visualiser ${VISUALISER_SRC}
        include/vclient.h
        include/Logger.h
        src/shared_libs/Logger.cpp
        src/shared_libs/vclient.cpp
        src/visualiser/wss.cpp
)

# Link dependencies for Visualiser
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
add_custom_command(TARGET AARNN POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/configure/simulation.conf"
        "$<TARGET_FILE_DIR:AARNN>/simulation.conf"
)

# Copy db_connection.conf into the same directory as the Visualiser executable
add_custom_command(TARGET Visualiser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/configure/db_connection.conf"
        "$<TARGET_FILE_DIR:Visualiser>/db_connection.conf"
)

