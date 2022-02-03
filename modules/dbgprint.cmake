# dbgprintf

if(DEFINED ENV{NGAGESDK})
    SET(NGAGESDK $ENV{NGAGESDK})
else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SRC_DIR          "${NGAGESDK}/modules/dbgprint/src")
set(DBGPRINT_INC_DIR "${SRC_DIR}")

set(dbgprint_sources
    "${SRC_DIR}/dbgprint.cpp")

add_library(dbgprint STATIC ${dbgprint_sources})

target_compile_definitions(
    dbgprint
    PUBLIC
    ${GCC_MODULE_DEFS})

target_include_directories(
    dbgprint
    PUBLIC
    ${SRC_DIR})
