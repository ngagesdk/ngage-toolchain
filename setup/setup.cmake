cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "Run this file as 'cmake -P ${CMAKE_CURRENT_LIST_FILE}'")
endif()

if(NOT DEFINED ENV{NGAGESDK})
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

file(TO_CMAKE_PATH "$ENV{NGAGESDK}" NGAGESDK)

set(SDK_DIR "${NGAGESDK}/sdk")
include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

if(POLICY CMP0168)
  cmake_policy(SET CMP0168 NEW)
endif()

set(DOWNLOAD_DIR "${SDK_DIR}/cache")
set(FETCHCONTENT_BASE_DIR "${DOWNLOAD_DIR}" CACHE PATH "FETCHCONTENT_BASE_DIR")

FetchContent_Populate(apps
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/apps.zip
    URL_HASH SHA1=6e89e554dd502e5b739230ccf2f7c43703867b61
    DOWNLOAD_DIR "${DOWNLOAD_DIR}"
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
    SOURCE_DIR "${SDK_DIR}/apps"
    BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/apps-build"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "skipping configure step."
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

FetchContent_Populate(extras
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/extras.zip
    URL_HASH SHA1=f5128881b77aaa3fcbe86d602629e51901a3e3b8
    DOWNLOAD_DIR "${DOWNLOAD_DIR}"
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
    SOURCE_DIR "${SDK_DIR}/extras"
    BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/extras-build"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "skipping configure step."
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

FetchContent_Populate(sdk
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/sdk.zip
    URL_HASH SHA1=556b8225ab781e507bf7acd5c9de271918972b28
    DOWNLOAD_DIR "${DOWNLOAD_DIR}"
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
    SOURCE_DIR "${SDK_DIR}/sdk"
    BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/sdk-build"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "skipping configure step."
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)

FetchContent_Populate(tools
    URL https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/tools.zip
    URL_HASH SHA1=a8e1ef7fb9dee6860b616c4348dc13838d6da3e6
    DOWNLOAD_DIR "${DOWNLOAD_DIR}"
    DOWNLOAD_NO_PROGRESS TRUE
    TLS_VERIFY TRUE
    SOURCE_DIR "${SDK_DIR}/tools"
    BUILD_DIR "${CMAKE_CURRENT_LIST_DIR}/tools-build"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "skipping configure step."
    BUILD_COMMAND cmake -E echo "Skipping build step."
    INSTALL_COMMAND cmake -E echo "Skipping install step."
)
