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

FetchContent_Populate(extras
  GIT_REPOSITORY https://github.com/ngagesdk/extras.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR}/extras)

FetchContent_Populate(sdk
  GIT_REPOSITORY https://github.com/ngagesdk/sdk.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR}/sdk)

FetchContent_Populate(tools
  GIT_REPOSITORY https://github.com/ngagesdk/tools.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR}/tools)