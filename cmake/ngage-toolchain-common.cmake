cmake_minimum_required(VERSION 3.19)

set(NGAGE 1)
set(NGAGESDK 1)

set(CMAKE_SYSTEM_NAME NGage)
set(CMAKE_SYSTEM_PROCESSOR ARMV4)

set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

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

set(CMAKE_MODULE_PATH  "${CMAKE_MODULE_PATH};${NGAGESDK}/cmake")

set(SDK_ROOT ${NGAGESDK}/sdk)
set(S60_SDK_ROOT ${SDK_ROOT}/sdk/6.1)
set(EPOC_PLATFORM ${S60_SDK_ROOT}/Shared/EPOC32)
set(EPOC_LIB ${S60_SDK_ROOT}/Series60/Epoc32/Release/armi/urel)
set(EPOC_EXTRAS ${SDK_ROOT}/extras)

set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagecc.bat")
set(CMAKE_C_LINKER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagecc.bat")
set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagec++.bat")
set(CMAKE_CXX_LINKER_LAUNCHER "${CMAKE_CURRENT_LIST_DIR}/ngagec++.bat")

set(CMAKE_OBJCOPY "${EPOC_PLATFORM}/gcc/bin/objcopy")
set(CMAKE_OBJDUMP "${EPOC_PLATFORM}/gcc/bin/objdump")

set(CMAKE_RANLIB "${EPOC_PLATFORM}/gcc/bin/ranlib.exe")
set(CMAKE_AR "${EPOC_PLATFORM}/gcc/bin/ar.exe")

set(NGAGE_CPPFLAGS "-DFUNCTION_NAME=__FUNCTION__ -D__NGAGE__=1 -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__ -D_UNICODE")
set(NGAGE_CPPFLAGS "${NGAGE_CPPFLAGS} -I ${EPOC_PLATFORM}/include -I ${EPOC_EXTRAS}/include -I ${S60_SDK_ROOT}/Series60/Epoc32/Include -I ${S60_SDK_ROOT}/Series60/Epoc32/Include/libc -I ${S60_SDK_ROOT}/Shared/EPOC32/ngagesdk/include")
set(NGAGE_CPPFLAGS "${NGAGE_CPPFLAGS} -s -fomit-frame-pointer -O2 -mthumb-interwork -pipe -nostdinc -mstructure-size-boundary=8")

set(NGAGE_CFLAGS "${NGAGE_CPPFLAGS} -fno-leading-underscore")
set(NGAGE_CXXFLAGS "${NGAGE_CPPFLAGS} -march=armv4t -Wno-ctor-dtor-privacy")

set(CMAKE_C_FLAGS_INIT "${NGAGE_CFLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${NGAGE_CXXFLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "")#-Wl,-e,_E32Startup -Wl,-u,_E32Startup")

set(CMAKE_C_STANDARD_LIBRARIES "${EPOC_LIB}/eexe.lib")
set(CMAKE_CXX_STANDARD_LIBRARIES "${EPOC_LIB}/eexe.lib")

if (NGAGE_LEGACY)
  set(CMAKE_C_COMPILER_ID_RUN TRUE)
  set(CMAKE_C_COMPILER_FORCED TRUE)
  set(CMAKE_C_COMPILER_WORKS TRUE)
endif()

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
ngagesdk_add_static_imported_library(libgcc_ngage "${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a")
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

cmake_policy(SET CMP0053 NEW)  # Ensures proper argument parsing.

# FIXME: build_dll is not implemented by ngagecc.py

function(build_dll TARGET EXTENSION UID1 UID2 UID3 LIBS)
  # Create new DefFile from in library
  add_custom_target(${TARGET}.def ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET}.a
    COMMAND ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --output-def ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.def ${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET}.a
  )

  build_dll_ex("${TARGET}" "${EXTENSION}" "${UID1}" "${UID2}" "${UID3}" "${LIBS}" "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.def")
endfunction()

function(build_dll_ex TARGET EXTENSION UID1 UID2 UID3 LIBS def_file)
  # Create new Export file from generated DefFle
  add_custom_target(${TARGET}_tmp.exp ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.def
    COMMAND ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --def ${def_file}
      --output-exp ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_tmp.exp --dllname ${TARGET}[${UID3}].${EXTENSION}
  )

  # Create new Base file
  add_custom_target(${TARGET}.bas ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_tmp.exp
    COMMAND ${EPOC_PLATFORM}/gcc/bin/ld -s -e _E32Dll -u _E32Dll ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_tmp.exp --dll
        --base-file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bas -o ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_tmp.${EXTENSION}
        ${EPOC_LIB}/edll.lib --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET}.a --no-whole-archive ${LIBS}
  )

  # Create new EXPORT file with def a
  add_custom_target(${TARGET}.exp ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bas
    COMMAND ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --def ${def_file} --dllname ${TARGET}[${UID3}].${EXTENSION}
          --base-file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bas --output-exp ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.exp
  )

  # Create new interface LIB file with def a
  add_custom_target(${TARGET}.lib ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.exp
    COMMAND ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --def ${def_file} --dllname ${TARGET}[${UID3}].${EXTENSION}
          --base-file ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bas --output-lib ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.lib
  )

  add_custom_target(${TARGET}.map ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.lib
    COMMAND ${EPOC_PLATFORM}/gcc/bin/ld -s -e _E32Dll -u _E32Dll --dll ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.exp
          -Map ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.map -o ${TARGET}_tmp.${EXTENSION} ${EPOC_LIB}/edll.lib
          --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET}.a --no-whole-archive ${LIBS}
  )

  add_custom_target(${TARGET}.${EXTENSION} ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.map
    COMMAND ${EPOC_PLATFORM}/Tools/petran ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_tmp.${EXTENSION}
          ${CMAKE_CURRENT_BINARY_DIR}/${source}.${EXTENSION} -nocall -uid1 ${UID1} -uid2 ${UID2} -uid3 ${UID3}
  )
endfunction()

function(pack_assets resource_dir resources)
  add_custom_target(data.pfs ALL
    WORKING_DIRECTORY ${resource_dir}
    COMMAND ${NGAGESDK}/sdk/tools/packer ${resources}
  )
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
  add_custom_target(
    ${basename}.RSS_Intermediate
    ALL
    DEPENDS
    ${source_dir}/${basename}.rss
    COMMAND
    ${EPOC_PLATFORM}/gcc/bin/cpp ${extra_args} -I${S60_SDK_ROOT}/Series60/Epoc32/Include -I${source_dir} -I${source_dir}/../inc ${source_dir}/${basename}.rss ${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate)

  add_custom_target(
    ${basename}.rsc
    ALL
    DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate
    COMMAND
    ${EPOC_PLATFORM}/Tools/rcomp -u -s${CMAKE_CURRENT_BINARY_DIR}/${basename}.RSS_Intermediate -h${CMAKE_CURRENT_BINARY_DIR}/${basename}.rsg -o${CMAKE_CURRENT_BINARY_DIR}/${basename}.rsc)
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
    ${NGAGESDK}/sdk/tools/genaif -u ${UID3} ${source_dir}/${basename}.aifspec ${CMAKE_CURRENT_BINARY_DIR}/${basename}.aif)
endfunction()

function(build_sis source_dir basename)
  add_custom_target(
    ${basename}.sis
    ALL
    DEPENDS
    ${source_dir}/${basename}.pkg
    WORKING_DIRECTORY
    ${source_dir}
    COMMAND
    ${EPOC_PLATFORM}/Tools/makesis ${source_dir}/${basename}.pkg ${CMAKE_CURRENT_BINARY_DIR}/${basename}.sis)
endfunction()
