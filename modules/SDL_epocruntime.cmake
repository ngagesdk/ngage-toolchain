# SDL_epocruntime

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../projects/SDL-1.2/src")

set(SDL_epocruntime_sources
    "${SRC_DIR}/SDL_epocruntime/src/SDL_epocruntime.cpp")

add_library(SDL_epocruntime STATIC ${SDL_epocruntime_sources})

target_compile_definitions(
    SDL_epocruntime
    PUBLIC
    ${GCC_MODULE_DEFS})

target_include_directories(
    SDL_epocruntime
    PUBLIC
    ${SRC_DIR}/SDL_epocruntime/inc)
