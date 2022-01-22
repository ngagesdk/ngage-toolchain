# SDLmain

set(GCC_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_DEFS      ${GCC_COMN_DEFS} ${GCC_MODE_DEFS})

set(SRC_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/../../projects/SDL-1.2/src")
set(SDL_DIR       "${SRC_DIR}/SDL")

set(SDLmain_sources
    "${SDL_DIR}/src/main/epoc/SDL_main.cpp")

add_library(SDLmain STATIC ${SDLmain_sources})

target_compile_definitions(
    SDLmain
    PUBLIC
    ${GCC_DEFS})

target_include_directories(
    SDLmain
    PUBLIC
    ${SDL_DIR}/include)
