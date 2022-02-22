# libtmx

include_guard(GLOBAL)

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

include(libxml2)
include(zlib)

set(LIBTMX_DIR     "${NGAGESDK}/modules/tmx")
set(SRC_DIR        "${LIBTMX_DIR}/src")
set(LIBTMX_INC_DIR "${SRC_DIR}")

set(libtmx_sources
    "${SRC_DIR}/tmx.c"
    "${SRC_DIR}/tmx_err.c"
    "${SRC_DIR}/tmx_hash.c"
    "${SRC_DIR}/tmx_mem.c"
    "${SRC_DIR}/tmx_utils.c"
    "${SRC_DIR}/tmx_xml.c")

add_library(tmx STATIC ${libtmx_sources})

target_compile_definitions(
    tmx
    PUBLIC
    WANT_ZLIB
    ${GCC_MODULE_DEFS})

target_compile_options(
    tmx
    PUBLIC
    -O3)

target_include_directories(
    tmx
    PUBLIC
    ${LIBXML2_INC_DIR}
    ${LIBTMX_INC_DIR}
    ${ZLIB_INC_DIR})
