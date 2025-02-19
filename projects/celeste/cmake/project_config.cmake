# Project configuration.

set(PROJECT_TITLE "celeste")

set(project_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/celeste.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/celeste_SDL3.c)

set(UID3 0x1000c37e) # game.exe UID
set(APP_UID 0x10005731) # celeste.app UID
set(APP_NAME "Celeste")
