cmake_minimum_required(VERSION 3.9.6)

project(wawt C CXX)

find_package(Threads)

set(CMAKE_BUILD_TYPE "Debug"
    CACHE STRING "Choose the type of build." FORCE)

set(CRYPTOPP_INCLUDE_DIR "/usr/include"
    CACHE FILEPATH "Install location for crypto++")

set(CRYPTOPP_LIBRARY_DIR "/usr/lib64"
    CACHE FILEPATH "Install location for crypto++")

set(SFML_INCLUDE_DIR "/usr/include"
    CACHE FILEPATH "Install location for SFML")

set(SFML_LIBRARY_DIR "/usr/lib64"
    CACHE FILEPATH "Install location for SFML")

add_compile_options(-std=c++1z -Wall -Wextra -Wshadow -pedantic -pthread)

include_directories(include ${CRYPTO_INCLUDE_DIR} ${SFML_INCLUDE_DIR})

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(LIBSUFFIX "-d")
endif()

#enable_testing()

add_subdirectory(doc)
add_subdirectory(reference_impl)
add_subdirectory(examples/tictacdoh)
add_subdirectory(src/wawt)
