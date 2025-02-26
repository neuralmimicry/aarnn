# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-src")
  file(MAKE_DIRECTORY "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-src")
endif()
file(MAKE_DIRECTORY
  "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-build"
  "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix"
  "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix/tmp"
  "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
  "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix/src"
  "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/pbisaacs/Developer/aarnn/cmake-build-release/_deps/json-subbuild/json-populate-prefix/src/json-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
