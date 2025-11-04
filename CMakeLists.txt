cmake_minimum_required(VERSION 3.31.6)
project(AARNN VERSION 1.0 LANGUAGES C CXX)

#––– 1) GLOBAL CONFIGURATION –––––––––––––––––––––––––––––––––––––––––––
# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Only set these policies if they exist in this CMake version
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)            # for DOWNLOAD_EXTRACT_TIMESTAMP
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)            # to silence the FindBoost warning
endif()
set(FETCHCONTENT_DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# Default build type (can be overridden by -DCMAKE_BUILD_TYPE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, etc.)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Ensure /usr/local/include is used before /usr/include
#set(CMAKE_INCLUDE_PATH /usr/local/include ${CMAKE_INCLUDE_PATH})

# Display the CMake module path
message(STATUS "CMake module path: ${CMAKE_MODULE_PATH}")

# Enable AddressSanitizer and UndefinedBehaviorSanitizer if requested
option(ENABLE_SANITIZERS "Enable Address/Undefined Behavior Sanitizers" OFF)
if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers: Address, Undefined")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

#––– 2) FETCH & PROVIDE HEADER-ONLY LIBRARIES –––––––––––––––––––––––––––
#include(FetchContent)
#FetchContent_Declare(
#        nlohmann_json
#        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
#)
#FetchContent_MakeAvailable(nlohmann_json)

#––– 3) DEPENDENCY RESOLUTION –––––––––––––––––––––––––––––––––––––––––––
find_package(Threads    REQUIRED)
find_package(MPI        REQUIRED)
find_package(PkgConfig  REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenGL     REQUIRED)
find_package(GLEW       REQUIRED)
find_package(Freetype   REQUIRED)
# Use pkg-config for libsndfile to improve portability on Debian/Ubuntu
pkg_check_modules(SNDFILE REQUIRED sndfile)
find_package(VTK REQUIRED COMPONENTS
        RenderingCore
        RenderingOpenGL2
        InteractionStyle
        RenderingFreeType
        FiltersSources
        IOXML
        RenderingContextOpenGL2
)

find_package(Boost   REQUIRED COMPONENTS filesystem system json)
find_package(OpenSSL REQUIRED)
find_package(CURL    REQUIRED)
find_package(OpenCV  REQUIRED COMPONENTS core imgproc highgui)
# CGAL (computational geometry used in utils.h and neuron geometry)
find_package(CGAL REQUIRED)

find_library(PQXX_LIB pqxx HINTS /usr/lib64)

# ALSA, PortAudio, FFmpeg, GStreamer via pkg-config
pkg_check_modules(ALSA      REQUIRED alsa)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(PULSE     REQUIRED libpulse libpulse-simple)
pkg_check_modules(FFMPEG    REQUIRED libavcodec libavformat libavutil libswscale libswresample)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-audio-1.0)
pkg_check_modules(FFTW3     REQUIRED fftw3)
pkg_check_modules(GMP       REQUIRED gmp)

#––– 4) COMMON MODULE –––––––––––––––––––––––––––––––––––––––––––––––––
file(GLOB_RECURSE COMMON_SRCS src/shared_libs/*.cpp)
add_library(common STATIC ${COMMON_SRCS})

target_link_libraries(common PUBLIC
        Threads::Threads
        VTK::CommonCore
        VTK::IOXML
        CURL::libcurl
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::InteractionStyle
        VTK::RenderingFreeType
        VTK::FiltersSources
        ${GMP_LIBRARIES}
        ${GMP_LDFLAGS_OTHER}
        Boost::json
        CGAL::CGAL
#        nlohmann_json::nlohmann_json
)
target_include_directories(common PUBLIC
#        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        ${GMP_INCLUDE_DIRS}
)

#––– 6) CORE NEURON MODEL ––––––––––––––––––––––––––––––––––––––––––––––
file(GLOB_RECURSE CORE_SRCS src/aarnn/*.cpp)
list(REMOVE_ITEM CORE_SRCS src/aarnn/aarnn.cpp)
add_library(aarnn_core STATIC
        ${CORE_SRCS}
        src/aarnn/ProcessorManager.cpp)

target_link_libraries(aarnn_core PUBLIC
        OpenSSL::SSL OpenSSL::Crypto
        ${PQXX_LIB}
        PostgreSQL::PostgreSQL
        ${OpenCV_LIBS}
        common
        Boost::filesystem Boost::system
)
target_include_directories(aarnn_core PUBLIC
        include
        ${OpenCV_INCLUDE_DIRS}
#        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include>
)

#––– 7) AUDIO PROCESSING MODULE ––––––––––––––––––––––––––––––––––––––––
file(GLOB_RECURSE AUDIO_SRCS src/audio/*.cpp)
list(REMOVE_ITEM AUDIO_SRCS src/audio/AuditorySolution.cpp)
add_library(audio_lib STATIC ${AUDIO_SRCS})

target_link_libraries(audio_lib PUBLIC
        Threads::Threads
        ${SNDFILE_LIBRARIES}
        ${ALSA_LIBRARIES}
        ${PORTAUDIO_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        CURL::libcurl
        ${FFTW3_LIBRARIES}
        ${PULSE_LIBRARIES}
        ${PULSE_LDFLAGS_OTHER}
        Boost::json
        #        nlohmann_json::nlohmann_json
        common
        aarnn_core
)
target_include_directories(audio_lib PUBLIC
        src/audio
        include
        ${SNDFILE_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${FFTW3_INCLUDE_DIRS}
        ${PULSE_INCLUDE_DIRS}
#        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include>
)

#––– 8) EXECUTABLE TARGETS –––––––––––––––––––––––––––––––––––––––––––––

add_executable(hello_world src/hello-world/sphere.cpp)
target_link_libraries(hello_world PRIVATE
        VTK::CommonCore
        VTK::IOXML
        VTK::RenderingContextOpenGL2
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::InteractionStyle
        VTK::RenderingFreeType
        VTK::FiltersSources
)

add_executable(vtk_test src/hello-world/vtk_test.cpp)
target_link_libraries(vtk_test PRIVATE
        VTK::CommonCore
        VTK::IOXML
        VTK::RenderingContextOpenGL2
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::InteractionStyle
        VTK::RenderingFreeType
        VTK::FiltersSources
)

add_executable(Audio src/audio/AuditorySolution.cpp)
target_link_libraries(Audio PRIVATE audio_lib)

add_executable(AARNN src/aarnn/aarnn.cpp)
target_link_libraries(AARNN PRIVATE aarnn_core audio_lib)

add_executable(Visualiser
        src/visualiser/visualiser.cpp
        src/visualiser/wss.cpp
)
target_include_directories(Visualiser
        BEFORE PRIVATE
        ${VTK_INCLUDE_DIRS}
#        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/nlohmann_json-src/include>
        PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Visualiser PRIVATE
        OpenGL::GL
        GLEW::GLEW
        VTK::CommonCore
        VTK::IOXML
        VTK::RenderingContextOpenGL2
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::InteractionStyle
        VTK::RenderingFreeType
        VTK::FiltersSources
        Boost::filesystem
        Boost::json
        #        nlohmann_json::nlohmann_json
        common
        aarnn_core
)

#––– 9) POST-BUILD CONFIG COPY –––––––––––––––––––––––––––––––––––––––––
foreach(_exe IN ITEMS Audio AARNN Visualiser)
    set(_src_conf ${CMAKE_SOURCE_DIR}/configure/${_exe}.conf)
    if(EXISTS ${_src_conf})
        add_custom_command(TARGET ${_exe} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_src_conf}
                $<TARGET_FILE_DIR:${_exe}>/${_exe}.conf
        )
    endif()
endforeach()
