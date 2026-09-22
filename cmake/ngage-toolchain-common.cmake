cmake_minimum_required(VERSION 3.19)

set(NGAGE 1)
set(NGAGESDK 1)

set(CMAKE_SYSTEM_NAME NGage)
set(CMAKE_SYSTEM_PROCESSOR ARMV4)

set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

# Compiler is working by default.
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

set(CMAKE_IMPORT_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_MODULE_PREFIX  "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

set(CMAKE_EXECUTABLE_SUFFIX     ".exe")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".lib")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_SHARED_MODULE_SUFFIX  ".dll")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")

set(CMAKE_DL_LIBS "")

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_LIST_DIR}")

if(NOT DEFINED ENV{NGAGESDK})
  message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

file(TO_CMAKE_PATH "$ENV{NGAGESDK}" NGAGESDK)

set(SDK_ROOT ${NGAGESDK}/sdk)
set(S60_SDK_ROOT ${SDK_ROOT}/sdk/6.1)
set(EPOC_PLATFORM ${S60_SDK_ROOT}/Shared/EPOC32)
set(EPOC_LIB ${S60_SDK_ROOT}/Series60/Epoc32/Release/armi/urel)
set(EPOC_EXTRAS ${SDK_ROOT}/extras)
set(EPOC_TOOLS ${EPOC_PLATFORM}/Tools)

set(NGAGE_LD_SYMLINKS "-L${S60_SDK_ROOT} -L${EPOC_PLATFORM} -L${EPOC_LIB}")

# Fix where cmake files for lib are placed
set(CMAKE_MODULE_PATH  "${CMAKE_MODULE_PATH};${EPOC_EXTRAS}/lib/cmake")

if(WIN32)
  set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagecc.bat")
  set(CMAKE_C_LINKER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagecc.bat")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagec++.bat")
  set(CMAKE_CXX_LINKER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagec++.bat")
endif()

if(WIN32)
  SET(EPOC_DLLTOOL "${EPOC_PLATFORM}/gcc/bin/dlltool")
  SET(EPOC_LD "${EPOC_PLATFORM}/gcc/bin/ld")
  SET(EPOC_CPP "${EPOC_PLATFORM}/gcc/bin/cpp")
  set(EPOC_AS "${EPOC_PLATFORM}/gcc/bin/as")
elseif(UNIX)
  set(EPOC_LD "${EPOC_PLATFORM}/gcc/bin/arm-epoc-pe-ld")
  set(EPOC_DLLTOOL "${EPOC_PLATFORM}/gcc/bin/arm-epoc-pe-dlltool")
  SET(EPOC_CPP "${EPOC_PLATFORM}/gcc/bin/arm-epoc-pe-cpp")
  set(EPOC_AS "${EPOC_PLATFORM}/gcc/bin/arm-epoc-pe-as")
  set(EPOC_PETRAN "${EPOC_TOOLS}/petran")
endif()

set(NGAGE_CPPFLAGS "-B${EPOC_PLATFORM}/gcc/arm-epoc-pe/bin -DFUNCTION_NAME=__FUNCTION__ -D__NGAGE__=1 -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__ -D_UNICODE")
set(NGAGE_CPPFLAGS "${NGAGE_CPPFLAGS} -I ${EPOC_PLATFORM}/include -I ${EPOC_EXTRAS}/include -I ${S60_SDK_ROOT}/Series60/Epoc32/Include -I ${S60_SDK_ROOT}/Series60/Epoc32/Include/libc -I ${S60_SDK_ROOT}/Shared/EPOC32/ngagesdk/include")
set(NGAGE_CPPFLAGS "${NGAGE_CPPFLAGS} -s -fomit-frame-pointer -O2 -mthumb-interwork -pipe -nostdinc -mstructure-size-boundary=8")

set(NGAGE_CFLAGS "${NGAGE_CPPFLAGS} -fno-leading-underscore")
set(NGAGE_CXXFLAGS "${NGAGE_CPPFLAGS} -march=armv4t -Wno-ctor-dtor-privacy")

set(CMAKE_C_FLAGS_INIT "${NGAGE_CFLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${NGAGE_CXXFLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "")

set(CMAKE_C_STANDARD_LIBRARIES "${EPOC_LIB}/eexe.lib")
set(CMAKE_CXX_STANDARD_LIBRARIES "${EPOC_LIB}/eexe.lib")

if(NGAGE_LEGACY)
  set(CMAKE_C_COMPILER_ID_RUN TRUE)
  set(CMAKE_C_COMPILER_FORCED TRUE)
  set(CMAKE_C_COMPILER_WORKS TRUE)
endif()

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_ID_RUN TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

function(ngagesdk_add_static_imported_library TARGET IMPORTED_LOCATION)
  if(NOT TARGET "${TARGET}")
    add_library(${TARGET} STATIC IMPORTED)
    set_property(TARGET ${TARGET} PROPERTY IMPORTED_LOCATION "${IMPORTED_LOCATION}")
  endif()
endfunction()

ngagesdk_add_static_imported_library(NRenderer "${EPOC_LIB}/NRenderer.lib")
ngagesdk_add_static_imported_library(3dtypes "${EPOC_LIB}/3dtypes.a")
ngagesdk_add_static_imported_library(cone "${EPOC_LIB}/cone.lib")
ngagesdk_add_static_imported_library(libgcc "${EPOC_PLATFORM}/gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a")
ngagesdk_add_static_imported_library(mediaclientaudiostream "${EPOC_LIB}/mediaclientaudiostream.lib")
ngagesdk_add_static_imported_library(charconv "${EPOC_LIB}/charconv.lib")
ngagesdk_add_static_imported_library(bitgdi "${EPOC_LIB}/bitgdi.lib")
ngagesdk_add_static_imported_library(euser "${EPOC_LIB}/euser.lib")
ngagesdk_add_static_imported_library(estlib "${EPOC_LIB}/estlib.lib")
ngagesdk_add_static_imported_library(ws32 "${EPOC_LIB}/ws32.lib")
ngagesdk_add_static_imported_library(hal "${EPOC_LIB}/hal.lib")
ngagesdk_add_static_imported_library(fbscli "${EPOC_LIB}/fbscli.lib")
ngagesdk_add_static_imported_library(efsrv "${EPOC_LIB}/efsrv.lib")
ngagesdk_add_static_imported_library(scdv "${EPOC_LIB}/scdv.lib")
ngagesdk_add_static_imported_library(gdi "${EPOC_LIB}/gdi.lib")

# TODO: change to one location.
if(WIN32)
  ngagesdk_add_static_imported_library(libgcc_ngage "${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a")
  ngagesdk_add_static_imported_library(SDL3 "${EPOC_EXTRAS}/lib/SDL3-static.lib")
  ngagesdk_add_static_imported_library(SDL3_mixer "${EPOC_EXTRAS}/lib/SDL3_mixer-static.lib")
elseif(UNIX)
  ngagesdk_add_static_imported_library(libgcc_ngage "${EPOC_PLATFORM}/ngagesdk/lib/libgcc_ngage.a")
  ngagesdk_add_static_imported_library(SDL3 "${EPOC_EXTRAS}/lib/libSDL3.a")
  ngagesdk_add_static_imported_library(SDL3_mixer "${EPOC_EXTRAS}/lib/libSDL3_mixer.a")
endif()

cmake_policy(SET CMP0053 NEW)  # Ensures proper argument parsing.

if(UNIX)
  # Multi-step EXECUTABLE link rule (replacement for ngagecc)
  set(CMAKE_C_LINK_EXECUTABLE
    "${EPOC_LD} -o <TARGET_BASE>_base.exe ${NGAGE_LD_SYMLINKS} -s -e _E32Startup -u _E32Startup -s <OBJECTS> <LINK_LIBRARIES> --base-file <TARGET_BASE>_base.bas"
    "${EPOC_DLLTOOL} -m arm_interwork --as ${EPOC_AS} --base-file <TARGET_BASE>_base.bas --output-exp <TARGET_BASE>.exp"
    "${EPOC_LD} -o <TARGET_BASE>_notran.exe ${NGAGE_LD_SYMLINKS} -s -e _E32Startup -u _E32Startup -s <OBJECTS> <LINK_LIBRARIES> game.exp"
    "${EPOC_PETRAN} <TARGET_BASE>_notran.exe <TARGET_BASE>.exe -nocall <LINK_FLAGS> -stack 500000 -heap 1000000 20000000"
  )
  set(CMAKE_CXX_LINK_EXECUTABLE ${CMAKE_C_LINK_EXECUTABLE})
endif()

function(build_dll LIB FILENAME EXTENSION UID1 UID2 UID3 LIBS)
  set(bin ${CMAKE_CURRENT_BINARY_DIR})

  # Create new DefFile from in library
  add_custom_command(
    OUTPUT ${bin}/${FILENAME}.def
    DEPENDS ${bin}/${LIB}.lib
    COMMAND ${EPOC_DLLTOOL} -m arm_interwork --output-def ${bin}/${FILENAME}.def ${bin}/${LIB}.lib)

  build_dll_ex("${LIB}" "${FILENAME}" "${EXTENSION}" "${UID1}" "${UID2}" "${UID3}" "${LIBS}" "${bin}/${FILENAME}.def")
endfunction()

function(build_dll_ex LIB FILENAME EXTENSION UID1 UID2 UID3 LIBS def_file)
  set(bin ${CMAKE_CURRENT_BINARY_DIR})

  add_custom_command(
    OUTPUT ${bin}/${FILENAME}_tmp.exp
    DEPENDS ${def_file}
    COMMAND ${EPOC_DLLTOOL} -m arm_interwork --def ${def_file} --as ${EPOC_AS}
      --output-exp ${bin}/${FILENAME}_tmp.exp --dllname ${FILENAME}[${UID3}].${EXTENSION})

  add_custom_command(
    OUTPUT ${bin}/${FILENAME}.bas ${bin}/${FILENAME}_tmp.${EXTENSION}
    DEPENDS ${bin}/${FILENAME}_tmp.exp
    COMMAND ${EPOC_LD} -s -e _E32Dll -u _E32Dll ${bin}/${FILENAME}_tmp.exp --dll
        --base-file ${bin}/${FILENAME}.bas -o ${bin}/${FILENAME}_tmp.${EXTENSION}
        ${EPOC_LIB}/edll.lib --whole-archive ${bin}/${LIB}.lib --no-whole-archive ${LIBS})

  add_custom_command(
    OUTPUT ${bin}/${FILENAME}.exp
    DEPENDS ${bin}/${FILENAME}.bas
    COMMAND ${EPOC_DLLTOOL} -m arm_interwork --def ${def_file} --as ${EPOC_AS} --dllname ${FILENAME}[${UID3}].${EXTENSION}
          --base-file ${bin}/${FILENAME}.bas --output-exp ${bin}/${FILENAME}.exp)

  add_custom_command(
    OUTPUT ${bin}/${FILENAME}_tmp.lib
    DEPENDS ${bin}/${FILENAME}.exp
    COMMAND ${EPOC_DLLTOOL} -m arm_interwork --def ${def_file} --as ${EPOC_AS} --dllname ${FILENAME}[${UID3}].${EXTENSION}
          --base-file ${bin}/${FILENAME}.bas --output-lib ${bin}/${FILENAME}_tmp.lib)

  add_custom_command(
    OUTPUT ${bin}/${FILENAME}.map
    DEPENDS ${bin}/${FILENAME}_tmp.lib
    COMMAND ${EPOC_LD} -s -e _E32Dll -u _E32Dll --dll ${bin}/${FILENAME}.exp
          -Map ${bin}/${FILENAME}.map -o ${FILENAME}_tmp.${EXTENSION} ${EPOC_LIB}/edll.lib
          --whole-archive ${bin}/${LIB}.lib --no-whole-archive ${LIBS})

  add_custom_command(
    OUTPUT ${bin}/${FILENAME}.${EXTENSION}
    DEPENDS ${bin}/${FILENAME}.map
    COMMAND ${EPOC_TOOLS}/petran ${bin}/${FILENAME}_tmp.${EXTENSION}
          ${bin}/${FILENAME}.${EXTENSION} -nocall -uid1 ${UID1} -uid2 ${UID2} -uid3 ${UID3})

  add_custom_target(${FILENAME}_${EXTENSION}_target ALL
    DEPENDS ${bin}/${FILENAME}.${EXTENSION})
endfunction()

function(pack_assets resource_dir resources)
  add_custom_target(data.pfs ALL
    WORKING_DIRECTORY ${resource_dir}
    COMMAND ${NGAGESDK}/sdk/tools/packer ${resources})
endfunction()

function(copy_file_ex main_dep source_dir dest_dir src_file dst_file)
  add_custom_command(
    TARGET
    ${main_dep}
    POST_BUILD
    COMMAND
    ${CMAKE_COMMAND} -E copy
    ${source_dir}/${src_file}
    ${dest_dir}/${dst_file})
endfunction()

function(copy_file main_dep source_dir dest_dir file)
  copy_file_ex(
    ${main_dep}
    ${source_dir}
    ${dest_dir}
    ${file}
    ${file})
endfunction()

function(install_file main_dep project_name source_dir file drive_letter)
  add_custom_command(
    TARGET
    ${main_dep}
    POST_BUILD
    COMMAND
    ${CMAKE_COMMAND} -E copy
    ${source_dir}/${file}
    ${drive_letter}:/system/apps/${project_name}/${file})
endfunction()

function(build_resource source_dir basename extra_args)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate
    COMMAND ${EPOC_CPP} ${extra_args} -I${S60_SDK_ROOT}/Series60/Epoc32/Include 
	          -I${source_dir} -I${source_dir}/../inc ${source_dir}/${basename}.rss ${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate)

  add_custom_target(
    ${basename}.rsc
    ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate
    COMMAND ${EPOC_TOOLS}/rcomp -u -s${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate 
	          -h${CMAKE_CURRENT_BINARY_DIR}/${basename}.rsg -o${CMAKE_CURRENT_BINARY_DIR}/${basename}.rsc)
endfunction()


function(build_aif source_dir basename UID3)
  add_custom_target(
    ${basename}.aif
    ALL
    DEPENDS
    ${source_dir}/${basename}.aifspec
    WORKING_DIRECTORY
    ${source_dir}
    COMMAND
    ${NGAGESDK}/sdk/tools/genaif -u ${UID3} ${basename}.aifspec ${CMAKE_CURRENT_BINARY_DIR}/${basename}.aif)
endfunction()

function(build_sis source_dir basename outputname)
  add_custom_target(
    ${basename}.sis
    ALL
    DEPENDS
    ${source_dir}/${basename}.pkg
    WORKING_DIRECTORY 
    ${source_dir}
    COMMAND
    ${EPOC_TOOLS}/makesis ${basename}.pkg ${CMAKE_CURRENT_BINARY_DIR}/${outputname}.sis)
endfunction()
