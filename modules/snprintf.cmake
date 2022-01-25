# snprintf

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SRC_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/../../projects/snprintf")
set(SNPRINTF_INC_DIR "${SRC_DIR}")

set(snprintf_sources
    "${SRC_DIR}/snprintf.cpp")

add_library(snprintf STATIC ${snprintf_sources})

target_compile_definitions(
    snprintf
    PUBLIC
    ${GCC_MODULE_DEFS})

target_include_directories(
    snprintf
    PUBLIC
    ${STDINT_INC_DIR}
    ${PRINTF_INC_DIR})
