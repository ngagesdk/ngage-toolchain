cmake_minimum_required(VERSION 3.10)

project(celeste C)

find_package(SDL3 REQUIRED)
find_package(SDL3_mixer REQUIRED)

add_executable(celeste
  src/main.c
  src/celeste.c
  src/celeste_SDL3.c
)
target_link_libraries(celeste PRIVATE SDL3_mixer::SDL3_mixer)
target_link_libraries(celeste PRIVATE SDL3::SDL3)

set_property(TARGET celeste PROPERTY C_STANDARD 99)

if(NGAGESDK)
  target_link_options(celeste PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(celeste PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(celeste PRIVATE "SHELL:-s UID3=0x1000c37e") # celeste.exe UID
endif()
