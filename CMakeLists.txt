cmake_minimum_required(VERSION 3.22)
project(SNMP)

# Dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/.cache/cpm CACHE PATH "Directory to cache CPM packages" FORCE)
include(cmake/CPM.cmake)

CPMAddPackage(
  NAME Boost
  VERSION 1.92.0
  URL https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.7z
  OPTIONS "BUILD_SHARED_LIBS OFF" 
          "BOOST_ENABLE_CMAKE ON"
          "BOOST_INCLUDE_LIBRARIES asio\\\;program_options")

# Library
add_library(snmp src/lib/snmp.cpp)
add_library(snmp::snmp ALIAS snmp)
target_compile_features(snmp PUBLIC cxx_std_17)
target_include_directories(snmp PUBLIC src/lib)
target_link_libraries(snmp PUBLIC Boost::asio)

# Example
add_executable(snmp.example src/examples/main.cpp)
target_link_libraries(snmp.example PRIVATE snmp::snmp)

# Tests
add_executable(snmp.tests
  src/tests/doctest.cpp)
target_link_libraries(snmp.example PRIVATE snmp::snmp)
