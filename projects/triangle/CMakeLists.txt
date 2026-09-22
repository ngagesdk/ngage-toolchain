cmake_minimum_required(VERSION 3.10)

project(triangle C CXX)

set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
set(UID2 0x100039ce) # KAppUidValue16, apadef.h
set(UID3     0x1000c37e) # game.exe UID
set(APP_UID  0x10005888) # launcher.app UID
set(APP_NAME "triangle")

set(SRC_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DATA_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res")

add_executable(game
  ${SRC_DIR}/main.c
)

target_link_libraries(game PRIVATE
  SDL3
  SDL3_mixer
  NRenderer
  3dtypes
  # cone
  libgcc
  libgcc_ngage
  mediaclientaudiostream
  charconv
  bitgdi
  euser
  estlib
  ws32
  hal
  fbscli
  efsrv
  # scdv
  gdi
 )

target_link_options(game PRIVATE -uid1 ${UID1} -uid2 ${UID2} -uid3 ${UID3})

set(launcher_libs
  ${EPOC_LIB}/euser.lib
  ${EPOC_LIB}/apparc.lib
  ${EPOC_LIB}/cone.lib
  ${EPOC_LIB}/eikcore.lib
  ${EPOC_LIB}/avkon.lib)

set(launcher_sources
  "${SRC_DIR}/ngage.cpp"
  "${SRC_DIR}/ngage_application.cpp"
  "${SRC_DIR}/ngage_appui.cpp"
  "${SRC_DIR}/ngage_appview.cpp"
  "${SRC_DIR}/ngage_document.cpp")

add_library(launcher STATIC ${launcher_sources})

target_compile_definitions(
  launcher
  PUBLIC
  __DLL__
  APP_NAME=${APP_NAME}
  ${GCC_DEFS}
  UID1=${UID1}
  UID2=${UID2}
  UID3=${APP_UID})

build_dll(launcher launcher app ${UID1} ${UID2} ${APP_UID} "${launcher_libs}")

build_resource(${CMAKE_SOURCE_DIR}/res launcher "")

build_aif(${CMAKE_SOURCE_DIR}/res launcher ${APP_UID})

build_sis(${CMAKE_SOURCE_DIR}/res launcher triangle_app)
