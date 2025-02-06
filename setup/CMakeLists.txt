cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{NGAGESDK})
    set(NGAGESDK $ENV{NGAGESDK})
else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

set(SDK_DIR ${NGAGESDK}/sdk)

include(FetchContent)

project(setup)

FetchContent_Declare(extras
  GIT_REPOSITORY https://github.com/ngagesdk/extras.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR}/extras)

FetchContent_Declare(sdk
  GIT_REPOSITORY https://github.com/ngagesdk/sdk.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR}/sdk)

FetchContent_Declare(tools
  GIT_REPOSITORY https://github.com/ngagesdk/tools.git
  GIT_TAG main
  SOURCE_DIR ${SDK_DIR}/tools)

FetchContent_MakeAvailable(extras)
FetchContent_MakeAvailable(sdk)
FetchContent_MakeAvailable(tools)
