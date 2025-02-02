cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{NGAGESDK})
  set(NGAGESDK $ENV{NGAGESDK})
  set(CMAKE_TOOLCHAIN_FILE ${NGAGESDK}/cmake/ngage-toolchain.cmake)
else()
  message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

project(minimal C CXX)

set(minimal_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

set(UID1 0x1000007a) # KExecutableImageUidValue, e32uid.h
set(UID2 0x100039ce) # KAppUidValue16, apadef.h
set(UID3 0x1000c55e) # minimal.exe UID -change me-

set(minimal_libs "")

#${EPOC_PLATFORM}/gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a
#${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a

add_library(minimal STATIC ${minimal_sources})
build_exe(minimal exe ${UID1} ${UID2} ${UID3} "${minimal_libs}")

add_dependencies(
  minimal.exe
  minimal)

target_compile_definitions(
  minimal
  PUBLIC
  __EXE__
  UID1=${UID1}
  UID2=${UID2}
  UID3=${UID3})
