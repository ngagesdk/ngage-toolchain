cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{NGAGESDK})
    SET(NGAGESDK $ENV{NGAGESDK})
else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

project(setup)

set(SDK_DIR  ${NGAGESDK}/sdk)
set(SDK_ROOT ${SDK_DIR}/6.1)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

ExternalProject_Add(SymbianSDK
    URL https://mupf.dev/S60_SDL_v1.2.zip
    URL_HASH SHA1=2430aabb46e660941dbfa5a71805a280f13dc660
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_ROOT}
    BUILD_BYPRODUCTS ${SDK_ROOT}/Shared/EPOC32/gcc/bin/gcc.exe
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step."
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy
    "${SDK_DIR}/../cmake/stub-sdk.cmake" ${SDK_ROOT}/CMakeLists.txt)

ExternalProject_Add(EKA2L1
    URL https://github.com/EKA2L1/EKA2L1/releases/download/continous/windows-latest.zip
    URL_HASH SHA1=5115e704fc2a6b5d371596d903d38283fd072004
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/eka2l1
    BUILD_BYPRODUCTS ${SDK_DIR}/eka2l1/eka2l1_qt.exe
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step."
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy
    "${SDK_DIR}/../cmake/stub-eka2l1.cmake" ${SDK_DIR}/eka2l1/CMakeLists.txt)
