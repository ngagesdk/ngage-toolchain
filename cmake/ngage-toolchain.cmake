include("${CMAKE_CURRENT_LIST_DIR}/ngage-toolchain-common.cmake")

set(CMAKE_C_COMPILER "${EPOC_PLATFORM}/ngagesdk/bin/arm-epoc-pe-gcc.exe")
set(CMAKE_CXX_COMPILER "${EPOC_PLATFORM}/gcc/bin/g++.exe")
