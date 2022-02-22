# zlib

include_guard(GLOBAL)

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(ZLIB_DIR     "${NGAGESDK}/modules/zlib")
set(SRC_DIR      "${ZLIB_DIR}")
set(ZLIB_INC_DIR "${ZLIB_DIR}")

set(zlib_sources
    "${SRC_DIR}/adler32.c"
    "${SRC_DIR}/compress.c"
    "${SRC_DIR}/crc32.c"
    "${SRC_DIR}/deflate.c"
    "${SRC_DIR}/gzclose.c"
    "${SRC_DIR}/gzlib.c"
    "${SRC_DIR}/gzread.c"
    "${SRC_DIR}/gzwrite.c"
    "${SRC_DIR}/inflate.c"
    "${SRC_DIR}/infback.c"
    "${SRC_DIR}/inftrees.c"
    "${SRC_DIR}/inffast.c"
    "${SRC_DIR}/trees.c"
    "${SRC_DIR}/uncompr.c"
    "${SRC_DIR}/zutil.c")

add_library(zlib STATIC ${zlib_sources})

target_compile_definitions(
    zlib
    PUBLIC
    NO_snprintf
    NO_vsnprintf
    ${GCC_MODULE_DEFS})

target_compile_options(
    zlib
    PUBLIC
    -O3)

target_include_directories(
    zlib
    PUBLIC
    ${ZLIB_INC_DIR})
