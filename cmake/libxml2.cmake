# libxml2

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(LIBXML2_DIR     "${NGAGESDK}/modules/libxml2")
set(SRC_DIR         "${LIBXML2_DIR}")
set(LIBXML2_INC_DIR "${LIBXML2_DIR}/include")

set(libxml2_sources
    "${SRC_DIR}/buf.c"
    "${SRC_DIR}/c14n.c"
    "${SRC_DIR}/catalog.c"
    "${SRC_DIR}/chvalid.c"
    "${SRC_DIR}/debugXML.c"
    "${SRC_DIR}/dict.c"
    "${SRC_DIR}/encoding.c"
    "${SRC_DIR}/entities.c"
    "${SRC_DIR}/error.c"
    "${SRC_DIR}/globals.c"
    "${SRC_DIR}/hash.c"
    "${SRC_DIR}/HTMLparser.c"
    "${SRC_DIR}/HTMLtree.c"
    "${SRC_DIR}/legacy.c"
    "${SRC_DIR}/list.c"
    "${SRC_DIR}/nanoftp.c"
    "${SRC_DIR}/nanohttp.c"
    "${SRC_DIR}/parser.c"
    "${SRC_DIR}/parserInternals.c"
    "${SRC_DIR}/pattern.c"
    "${SRC_DIR}/relaxng.c"
    "${SRC_DIR}/SAX.c"
    "${SRC_DIR}/SAX2.c"
    "${SRC_DIR}/schematron.c"
    "${SRC_DIR}/threads.c"
    "${SRC_DIR}/tree.c"
    "${SRC_DIR}/uri.c"
    "${SRC_DIR}/valid.c"
    "${SRC_DIR}/xinclude.c"
    "${SRC_DIR}/xlink.c"
    "${SRC_DIR}/xmlIO.c"
    "${SRC_DIR}/xmlmemory.c"
    "${SRC_DIR}/xmlmodule.c"
    "${SRC_DIR}/xmlreader.c"
    "${SRC_DIR}/xmlregexp.c"
    "${SRC_DIR}/xmlsave.c"
    "${SRC_DIR}/xmlschemas.c"
    "${SRC_DIR}/xmlschemastypes.c"
    "${SRC_DIR}/xmlstring.c"
    "${SRC_DIR}/xmlunicode.c"
    "${SRC_DIR}/xmlwriter.c"
#    "${SRC_DIR}/xpath.c"
    "${SRC_DIR}/xpointer.c"
    "${SRC_DIR}/xzlib.c")

add_library(xml2 STATIC ${libxml2_sources})

target_compile_definitions(
    xml2
    PUBLIC
    ${GCC_MODULE_DEFS})

target_compile_options(
    xml2
    PUBLIC
    -O3)

target_include_directories(
    xml2
    PUBLIC
    ${LIBXML2_INC_DIR})
