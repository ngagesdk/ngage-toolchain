# cwalk

include_guard(GLOBAL)

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(CWALK_DIR     "${NGAGESDK}/modules/cwalk")
set(SRC_DIR       "${CWALK_DIR}/src")
set(CWALK_INC_DIR "${CWALK_DIR}/include")

set(cwalk_sources "${SRC_DIR}/cwalk.c")

add_library(cwalk STATIC ${cwalk_sources})

target_compile_definitions(
    cwalk
    PUBLIC
    ${GCC_MODULE_DEFS})

target_compile_options(
    cwalk
    PUBLIC
    -O3)

target_include_directories(
    cwalk
    PUBLIC
    ${CWALK_INC_DIR})
