# Do not modify this file unless absolutely necessary.
# All project-specific settings can be found in the file:
# project_config.cmake

cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_FOR_WINDOWS "Build for Windows" OFF)

if (BUILD_FOR_WINDOWS)
  project(celeste C CXX)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

  set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${EXPORT_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${EXPORT_DIR})

  add_executable(celeste WIN32 ${project_sources})
  add_compile_definitions(
      NGAGE_DEBUG
      DOXYGEN_SHOULD_IGNORE_THIS) # Skips assert.
  target_link_libraries(celeste ${EXPORT_DIR}/SDL3-3.2.0/lib/x86/SDL3.lib)
  target_include_directories(celeste PUBLIC ${EXPORT_DIR}/SDL3-3.2.0/include)

  include(FetchContent)

  cmake_policy(SET CMP0135 OLD)

  FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-devel-3.2.0-VC.zip
    URL_HASH SHA1=d382072cd045d74a802541bf8d0b13e21ffed740
    DOWNLOAD_NO_PROGRESS true
    TLS_VERIFY true
    SOURCE_DIR ${EXPORT_DIR}/SDL3-3.2.0/)

  FetchContent_MakeAvailable(SDL3)

  add_custom_command(
    TARGET celeste POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${EXPORT_DIR}/SDL3-3.2.0/lib/x86/SDL3.dll ${EXPORT_DIR})

else()

  if(DEFINED ENV{NGAGESDK})
    set(NGAGESDK $ENV{NGAGESDK})
    set(CMAKE_TOOLCHAIN_FILE ${NGAGESDK}/cmake/ngage-toolchain.cmake)
  else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
  endif()

  project(game C CXX)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/project_config.cmake)

  add_compile_definitions(__NGAGE__)

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

  set(celeste_libs
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/apparc.lib
    ${EPOC_LIB}/cone.lib
    ${EPOC_LIB}/eikcore.lib
    ${EPOC_LIB}/avkon.lib)

  set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
  set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res")

  set(celeste_sources
    "${SRC_DIR}/ngage.cpp"
    "${SRC_DIR}/ngage_application.cpp"
    "${SRC_DIR}/ngage_appui.cpp"
    "${SRC_DIR}/ngage_appview.cpp"
    "${SRC_DIR}/ngage_document.cpp")

  add_library(game STATIC ${project_sources})
  add_library(celeste STATIC ${celeste_sources})
  build_exe_static(game exe ${UID1} ${UID2} ${UID3} "${game_static_libs}" "${game_libs}")
  build_dll(celeste app ${UID1} ${UID2} ${APP_UID} "${celeste_libs}")

  build_aif(${RESOURCE_DIR} celeste ${APP_UID})
  build_resource(${RESOURCE_DIR} celeste "")
  build_sis(${RESOURCE_DIR} celeste)

  add_dependencies(
    celeste.sis
    game.exe
    celeste.aif
    celeste.app
    celeste.rsc)

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
    celeste.app
    celeste)

  target_compile_definitions(
    celeste
    PUBLIC
    __DLL__
    APP_NAME=${APP_NAME}
    ${GCC_DEFS}
    UID1=${UID1}
    UID2=${UID2}
    UID3=${APP_UID})

  target_compile_options(
    celeste
    PUBLIC
    -O3)

  target_include_directories(
    celeste
    PUBLIC
    ${SRC_DIR})

endif()
