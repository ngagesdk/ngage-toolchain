set(CMAKE_SYSTEM_NAME Generic)

if(DEFINED ENV{NGAGESDK})
  SET(NGAGESDK $ENV{NGAGESDK})
else()
  message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
endif()

set(CMAKE_MODULE_PATH  "${CMAKE_MODULE_PATH};${NGAGESDK}/cmake")

set(SDK_ROOT ${NGAGESDK}/sdk)
set(S60_SDK_ROOT ${SDK_ROOT}/sdk/6.1)
set(EPOC_PLATFORM ${S60_SDK_ROOT}/Shared/EPOC32)
set(EPOC_LIB ${S60_SDK_ROOT}/Series60/Epoc32/Release/armi/urel)
set(EXTRA_LIB ${SDK_ROOT}/extras/lib)

set(CMAKE_C_COMPILER ${EPOC_PLATFORM}/ngagesdk/bin/arm-epoc-pe-gcc.exe)
set(CMAKE_CXX_COMPILER ${EPOC_PLATFORM}/gcc/bin/g++.exe)
set(CMAKE_OBJCOPY ${EPOC_PLATFORM}/gcc/bin/objcopy.exe)
set(CMAKE_OBJDUMP ${EPOC_PLATFORM}/gcc/bin/objdump.exe)

set(CMAKE_RANLIB ${EPOC_PLATFORM}/gcc/bin/ranlib.exe)
set(CMAKE_AR ${EPOC_PLATFORM}/gcc/bin/ar.exe)

set(CMAKE_C_COMPILER_ID_RUN TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_WORKS  TRUE)

set(CMAKE_CXX_COMPILER_ID_RUN TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

add_compile_definitions(
    __NGAGE__=1)

include_directories(
    SYSTEM
    ${EPOC_PLATFORM}/include
    ${S60_SDK_ROOT}/Series60/Epoc32/Include
    ${S60_SDK_ROOT}/Series60/Epoc32/Include/libc)

set(DEBUG_FLAGS "-save-temps")
set(CORE_FLAGS "-s -fomit-frame-pointer -O2 -mthumb-interwork -pipe -nostdinc -Wall -Wno-unknown-pragmas -Wno-switch -mstructure-size-boundary=8")

set(CMAKE_C_FLAGS "${CORE_FLAGS} -std=c99 -fno-leading-underscore" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS ${CORE_FLAGS} -march=armv4t -Wno-ctor-dtor-privacy CACHE STRING "cxx compiler flags")

function(build_exe_static source file_ext uid1 uid2 uid3 static_libs libs)
    add_custom_target(
        ${source}.bas
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/ld -s -e _E32Startup -u _E32Startup --base-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas -o ${CMAKE_CURRENT_BINARY_DIR}/${source}_tmp.${file_ext} ${EPOC_LIB}/eexe.lib --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a ${static_libs} --no-whole-archive ${libs})

    add_custom_target(
        ${source}.exp
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --base-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas --output-exp ${CMAKE_CURRENT_BINARY_DIR}/${source}.exp)

    add_custom_target(
        ${source}.${file_ext}_Intermediate
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.exp
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/ld -s -e _E32Startup -u _E32Startup ${CMAKE_CURRENT_BINARY_DIR}/${source}.exp -Map ${CMAKE_CURRENT_BINARY_DIR}/${source}.map -o ${CMAKE_CURRENT_BINARY_DIR}/${source}.${file_ext}_Intermediate ${EPOC_LIB}/eexe.lib --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a ${static_libs} --no-whole-archive ${libs})

    add_custom_target(
        ${source}.${file_ext}
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.${file_ext}_Intermediate
        COMMAND
        ${EPOC_PLATFORM}/Tools/petran ${CMAKE_CURRENT_BINARY_DIR}/${source}.${file_ext}_Intermediate ${CMAKE_CURRENT_BINARY_DIR}/${source}.${file_ext} -nocall -uid1 ${uid1} -uid2 ${uid2} -uid3 ${uid3} -stack 500000 -heap 1000000 20000000)
endfunction()

function(build_exe source file_ext uid1 uid2 uid3 libs)
    build_exe_static(${source} exe ${uid1} ${uid2} ${uid3} "" "${libs}")
endfunction()

function(build_dll source file_ext uid1 uid2 uid3 libs)
    # Create new DefFile from in library
    add_custom_target(
        ${source}.def
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --output-def ${CMAKE_CURRENT_BINARY_DIR}/${source}.def ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a)

    build_dll_ex("${source}" "${file_ext}" "${uid1}" "${uid2}" "${uid3}" "${libs}" "${CMAKE_CURRENT_BINARY_DIR}/${source}.def")
endfunction()

function(build_dll_ex source file_ext uid1 uid2 uid3 libs def_file)
    # Create new Export file from generated DefFle
    add_custom_target(
        ${source}_tmp.exp
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.def
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --def ${def_file} --output-exp ${CMAKE_CURRENT_BINARY_DIR}/${source}_tmp.exp --dllname ${source}[${uid3}].${file_ext})

    # Create new Base file
    add_custom_target(
        ${source}.bas
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}_tmp.exp
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/ld -s -e _E32Dll -u _E32Dll ${CMAKE_CURRENT_BINARY_DIR}/${source}_tmp.exp --dll --base-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas -o ${CMAKE_CURRENT_BINARY_DIR}/${source}_tmp.${file_ext} ${EPOC_LIB}/edll.lib --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a --no-whole-archive ${libs})

    # Create new EXPORT file with def a
    add_custom_target(
        ${source}.exp
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --def ${def_file} --dllname ${source}[${uid3}].${file_ext} --base-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas --output-exp ${CMAKE_CURRENT_BINARY_DIR}/${source}.exp)

    # Create new interface LIB file with def a
    add_custom_target(
        ${source}.lib
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.exp
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/dlltool -m arm_interwork --def ${def_file} --dllname ${source}[${uid3}].${file_ext} --base-file ${CMAKE_CURRENT_BINARY_DIR}/${source}.bas --output-lib ${CMAKE_CURRENT_BINARY_DIR}/${source}.lib)

    add_custom_target(
        ${source}.map
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.lib
        COMMAND
        ${EPOC_PLATFORM}/gcc/bin/ld -s -e _E32Dll -u _E32Dll --dll ${CMAKE_CURRENT_BINARY_DIR}/${source}.exp -Map ${CMAKE_CURRENT_BINARY_DIR}/${source}.map -o ${source}_tmp.${file_ext} ${EPOC_LIB}/edll.lib --whole-archive ${CMAKE_CURRENT_BINARY_DIR}/lib${source}.a --no-whole-archive ${libs})

    add_custom_target(
        ${source}.${file_ext}
        ALL
        DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/${source}.map
        COMMAND
        ${EPOC_PLATFORM}/Tools/petran ${CMAKE_CURRENT_BINARY_DIR}/${source}_tmp.${file_ext} ${CMAKE_CURRENT_BINARY_DIR}/${source}.${file_ext} -nocall -uid1 ${uid1} -uid2 ${uid2} -uid3 ${uid3})
endfunction()

function(pack_assets resource_dir resources)
    add_custom_target(
        data.pfs
        ALL
        WORKING_DIRECTORY
        ${resource_dir}
        COMMAND
        ${NGAGESDK}/sdk/tools/packer ${resources})
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

function(build_aif source_dir basename uid3)
    add_custom_target(
        ${basename}.aif
        ALL
        DEPENDS
        ${source_dir}/${basename}.aifspec
        WORKING_DIRECTORY
        ${source_dir}
        COMMAND
        ${NGAGESDK}/sdk/tools/genaif -u ${uid3} ${source_dir}/${basename}.aifspec ${CMAKE_CURRENT_BINARY_DIR}/${basename}.aif)
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
