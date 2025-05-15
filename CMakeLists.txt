cmake_minimum_required(VERSION 3.15)
project(AARNN VERSION 1.0 LANGUAGES C CXX)

#––– 1) GLOBAL CONFIGURATION –––––––––––––––––––––––––––––––––––––––––––
# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0135 NEW)            # for DOWNLOAD_EXTRACT_TIMESTAMP
set(FETCHCONTENT_DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
cmake_policy(SET CMP0167 NEW)            # to silence the FindBoost warning

# Default build type (can be overridden by -DCMAKE_BUILD_TYPE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, etc.)" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable AddressSanitizer and UndefinedBehaviorSanitizer if requested
option(ENABLE_SANITIZERS "Enable Address/Undefined Behavior Sanitizers" OFF)
if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers: Address, Undefined")
    add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

#––– 2) FETCH & PROVIDE HEADER-ONLY LIBRARIES –––––––––––––––––––––––––––
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

#––– 3) DEPENDENCY RESOLUTION –––––––––––––––––––––––––––––––––––––––––––
find_package(Threads    REQUIRED)
find_package(MPI        REQUIRED)
find_package(PkgConfig  REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenGL     REQUIRED)
find_package(GLEW       REQUIRED)
find_package(Freetype   REQUIRED)
find_package(SndFile    CONFIG REQUIRED)
find_package(VTK REQUIRED COMPONENTS
        RenderingOpenGL2
        InteractionStyle
        RenderingFreeType
        FiltersSources
)
find_package(Boost   REQUIRED COMPONENTS system filesystem)
find_package(OpenSSL REQUIRED)
find_package(CURL    REQUIRED)
find_package(OpenCV  REQUIRED COMPONENTS core imgproc highgui)

find_library(PQXX_LIB pqxx HINTS /usr/lib64)

# ALSA, PortAudio, FFmpeg, GStreamer via pkg-config
pkg_check_modules(ALSA      REQUIRED alsa)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
pkg_check_modules(PULSE     REQUIRED libpulse libpulse-simple)
pkg_check_modules(FFMPEG    REQUIRED libavcodec libavformat libavutil libswscale libswresample)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0 gstreamer-audio-1.0)
pkg_check_modules(FFTW3     REQUIRED fftw3)
pkg_check_modules(GMP       REQUIRED   gmp)

#––– 4) COMMON MODULE –––––––––––––––––––––––––––––––––––––––––––––––––
file(GLOB_RECURSE COMMON_SRCS src/shared_libs/*.cpp)
add_library(common STATIC ${COMMON_SRCS})
target_include_directories(common PUBLIC include ${GMP_INCLUDE_DIRS})
target_link_libraries(common PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
        VTK::RenderingOpenGL2
        VTK::InteractionStyle
        VTK::RenderingFreeType
        VTK::FiltersSources
        ${GMP_LIBRARIES}
        ${GMP_LDFLAGS_OTHER}
)

#––– 5) CORE NEURON MODEL ––––––––––––––––––––––––––––––––––––––––––––––
file(GLOB_RECURSE CORE_SRCS src/aarnn/*.cpp)
list(REMOVE_ITEM CORE_SRCS src/aarnn/main.cpp)
add_library(aarnn_core STATIC ${CORE_SRCS})
target_include_directories(aarnn_core PUBLIC include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(aarnn_core PUBLIC
        common
        Boost::filesystem Boost::system
        OpenSSL::SSL OpenSSL::Crypto
        ${PQXX_LIB}
        PostgreSQL::PostgreSQL
        ${OpenCV_LIBS}
)

#––– 6) AUDIO PROCESSING MODULE ––––––––––––––––––––––––––––––––––––––––
file(GLOB_RECURSE AUDIO_SRCS src/audio/*.cpp)
list(REMOVE_ITEM AUDIO_SRCS src/audio/AuditorySolution.cpp)
add_library(audio_lib STATIC ${AUDIO_SRCS})
target_include_directories(audio_lib PUBLIC
        src/audio
        include
        ${GSTREAMER_INCLUDE_DIRS}
        ${FFTW3_INCLUDE_DIRS}
        ${PULSE_INCLUDE_DIRS})
target_link_libraries(audio_lib PUBLIC
        aarnn_core
        common
        Threads::Threads
        nlohmann_json::nlohmann_json
        SndFile::sndfile
        ${ALSA_LIBRARIES}
        ${PORTAUDIO_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        CURL::libcurl
        ${FFTW3_LIBRARIES}
        ${PULSE_LIBRARIES}
        ${PULSE_LDFLAGS_OTHER}
)

#––– 7) EXECUTABLE TARGETS –––––––––––––––––––––––––––––––––––––––––––––
add_executable(Audio src/audio/AuditorySolution.cpp)
target_link_libraries(Audio PRIVATE audio_lib)

add_executable(AARNN src/aarnn/main.cpp)
target_link_libraries(AARNN PRIVATE aarnn_core audio_lib)

add_executable(Visualiser
        src/visualiser/visualiser.cpp
        src/visualiser/wss.cpp
)
target_link_libraries(Visualiser PRIVATE
        common aarnn_core
        Boost::filesystem
        OpenGL::GL GLEW::GLEW
)

#––– 8) POST-BUILD CONFIG COPY –––––––––––––––––––––––––––––––––––––––––
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
