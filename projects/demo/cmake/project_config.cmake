# Project configuration.

set(PROJECT_TITLE "demo")

set(project_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/celeste.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fixedp.cpp)

set(UID3 0x10005799) # game.exe UID
set(APP_UID 0x10005800) # launcher.app UID
set(APP_NAME "demo")
