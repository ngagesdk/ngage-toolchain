cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{NGAGESDK})
    SET(NGAGESDK $ENV{NGAGESDK})
else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

set(SDK_DIR ${NGAGESDK}/sdk)

include(FetchContent)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  cmake_policy(SET CMP0135 NEW)
endif()

project(setup)

include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

FetchContent_Declare(sdk
  GIT_REPOSITORY https://github.com/ngagesdk/sdk.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR})

FetchContent_MakeAvailable(sdk)

ExternalProject_Add(apps
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/apps.zip
    URL_HASH SHA1=6e89e554dd502e5b739230ccf2f7c43703867b61
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/apps
    BUILD_BYPRODUCTS ${SDK_DIR}/apps/FExplorer_1.17.sis
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step.")

ExternalProject_Add(extras
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/extras.zip
    URL_HASH SHA1=2cc27486ef5cecca1d4b230cf1fa9ee31fc36390
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/extras
    BUILD_BYPRODUCTS ${SDK_DIR}/extras/lib/libSDL3.a
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step.")

ExternalProject_Add(tools
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/tools.zip
    URL_HASH SHA1=a8e1ef7fb9dee6860b616c4348dc13838d6da3e6
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/tools
    BUILD_BYPRODUCTS ${SDK_DIR}/tools/genaif.exe
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step.")
