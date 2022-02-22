# SDLmain

include_guard(GLOBAL)

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SRC_DIR   "${NGAGESDK}/modules/SDL-1.2/src")
set(SDL12_DIR "${SRC_DIR}/SDL")

set(SDLmain_sources
    "${SDL12_DIR}/src/main/epoc/SDL_main.cpp")

add_library(SDLmain STATIC ${SDLmain_sources})

target_compile_definitions(
    SDLmain
    PUBLIC
    ${GCC_MODULE_DEFS})

target_include_directories(
    SDLmain
    PUBLIC
    ${SDL12_DIR}/include)
