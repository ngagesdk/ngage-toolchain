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

ExternalProject_Add(apps
    URL https://media.githubusercontent.com/media/ngagesdk/ngagesdk.github.io/master/files/apps.zip
    URL_HASH SHA1=6e89e554dd502e5b739230ccf2f7c43703867b61
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/apps
    BUILD_BYPRODUCTS ${SDK_DIR}/apps/FExplorer_1.17.sis
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step.")

ExternalProject_Add(sdk
    URL https://media.githubusercontent.com/media/ngagesdk/ngagesdk.github.io/master/files/sdk.zip
    URL_HASH SHA1=21dc8078372d11489b14e52cab805e9c8c185dba
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/sdk
    BUILD_BYPRODUCTS ${SDK_DIR}/sdk/6.1/Shared/EPOC32/gcc/bin/gcc.exe
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step.")

ExternalProject_Add(tools
    URL https://media.githubusercontent.com/media/ngagesdk/ngagesdk.github.io/master/files/tools.zip
    URL_HASH SHA1=a8e1ef7fb9dee6860b616c4348dc13838d6da3e6
    DOWNLOAD_DIR ${SDK_DIR}/cache
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${SDK_DIR}/tools
    BUILD_BYPRODUCTS ${SDK_DIR}/tools/genaif.exe
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step.")
