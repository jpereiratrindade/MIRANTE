# Ecossistema SisTer — MIRANTE
# C++26 Application & Server Build Configuration
# Copyright (C) 2026 José Pedro Trindade

cmake_minimum_required(VERSION 3.25)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(mirante VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated-declarations)

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/third_party
  ${CMAKE_SOURCE_DIR}/third_party/httplib
  ${CMAKE_SOURCE_DIR}/third_party/nlohmann
)

# Core Library
add_library(mirante_core STATIC
  src/domain/entities.cpp
  src/domain/invariants.cpp
  src/storage/database.cpp
  src/assist/ollama_client.cpp
)

target_link_libraries(mirante_core PUBLIC
  Threads::Threads
  OpenSSL::Crypto
  OpenSSL::SSL
  SQLite3::SQLite3
)

# Main Executable
add_executable(mirante-server
  src/main.cpp
  src/http/server.cpp
)

target_link_libraries(mirante-server PRIVATE
  mirante_core
)

# Testing
include(CTest)
if(BUILD_TESTING)
  add_executable(mirante-tests
    tests/test_invariants.cpp
  )
  target_link_libraries(mirante-tests PRIVATE
    mirante_core
  )
  add_test(NAME mirante.invariants COMMAND mirante-tests)
endif()
