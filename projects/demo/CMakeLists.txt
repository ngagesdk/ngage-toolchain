# Do not modify this file unless absolutely necessary.
# All project-specific settings can be found in the file:
# project_config.cmake

cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(DEFINED ENV{NGAGESDK})
    set(NGAGESDK $ENV{NGAGESDK})
    set(CMAKE_TOOLCHAIN_FILE ${NGAGESDK}/cmake/ngage-toolchain.cmake)
else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

project(game C CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
set(UID2 0x100039ce) # KAppUidValue16, apadef.h

set(GCC_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_DEFS ${GCC_COMN_DEFS} ${GCC_MODE_DEFS})

set(game_static_libs
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libSDL3.a)

set(game_libs
    ${EPOC_LIB}/NRenderer.lib
    ${EPOC_LIB}/3dtypes.a
    ${EPOC_LIB}/cone.lib
    ${EPOC_PLATFORM}/gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a
    ${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a
    ${EPOC_LIB}/mediaclientaudiostream.lib
    ${EPOC_LIB}/charconv.lib
    ${EPOC_LIB}/bitgdi.lib
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/estlib.lib
    ${EPOC_LIB}/ws32.lib
    ${EPOC_LIB}/hal.lib
    ${EPOC_LIB}/fbscli.lib
    ${EPOC_LIB}/efsrv.lib
    ${EPOC_LIB}/scdv.lib
    ${EPOC_LIB}/gdi.lib)

set(demo_libs
  ${EPOC_LIB}/euser.lib
  ${EPOC_LIB}/apparc.lib
  ${EPOC_LIB}/cone.lib
  ${EPOC_LIB}/eikcore.lib
  ${EPOC_LIB}/avkon.lib)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res")

set(demo_sources
  "${SRC_DIR}/ngage.cpp"
  "${SRC_DIR}/ngage_application.cpp"
  "${SRC_DIR}/ngage_appui.cpp"
  "${SRC_DIR}/ngage_appview.cpp"
  "${SRC_DIR}/ngage_document.cpp")

add_library(game STATIC ${project_sources})
add_library(demo STATIC ${demo_sources})
build_exe_static(game exe ${UID1} ${UID2} ${UID3} "${game_static_libs}" "${game_libs}")
build_dll(demo app ${UID1} ${UID2} ${APP_UID} "${demo_libs}")

build_aif(${RESOURCE_DIR} demo ${APP_UID})
build_resource(${RESOURCE_DIR} demo "")
build_sis(${RESOURCE_DIR} demo)

add_dependencies(
    demo.sis
    game.exe
    demo.aif
    demo.app
    demo.rsc)

add_dependencies(
    game.exe
    game)

target_compile_definitions(
    game
    PUBLIC
    __EXE__
    FUNCTION_NAME=__FUNCTION__
    ${GCC_DEFS}
    UID1=${UID1}
    UID2=${UID2}
    UID3=${UID3})

target_compile_options(
    game
    PUBLIC
    -O2)

target_include_directories(
    game
    PUBLIC
    ${SRC_DIR}
    ${SRC_DIR}/SDL3)

add_dependencies(
  demo.app
  demo)

target_compile_definitions(
  demo
  PUBLIC
  __DLL__
  APP_NAME=${APP_NAME}
  ${GCC_DEFS}
  UID1=${UID1}
  UID2=${UID2}
  UID3=${APP_UID})

target_compile_options(
  demo
  PUBLIC
  -O3)

target_include_directories(
  demo
  PUBLIC
  ${SRC_DIR})
