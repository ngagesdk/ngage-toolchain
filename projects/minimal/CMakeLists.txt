cmake_minimum_required(VERSION 3.10)

project(minimal C)

add_executable(minimal
  src/main.c
)

if(NGAGESDK)
    target_link_options(minimal PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
    target_link_options(minimal PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
    target_link_options(minimal PRIVATE "SHELL:-s UID3=0x1000c55e") # minimal.exe UID -change me-
endif()
