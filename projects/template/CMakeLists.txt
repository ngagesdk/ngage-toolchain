cmake_minimum_required(VERSION 3.10)

project(template C)

add_executable(template
  src/main.c
)

set_property(TARGET template PROPERTY C_STANDARD 99)

if(NGAGESDK)
  target_link_options(template PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(template PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(template PRIVATE "SHELL:-s UID3=0x1000c37e") # template.exe UID

  target_link_libraries(template PRIVATE
    ${EPOC_EXTRAS}/lib/armi/urel/SDL3_mixer.lib
    ${EPOC_EXTRAS}/lib/armi/urel/SDL3.lib
    ${EPOC_LIB}/NRenderer.lib
    ${EPOC_LIB}/3dtypes.a
    ${EPOC_LIB}/cone.lib
    ${EPOC_PLATFORM}/gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a
    ${EPOC_PLATFORM}/ngagesdk/lib/gcc/arm-epoc-pe/4.6.4/libgcc_ngage.a
    ${EPOC_LIB}/mediaclientaudiostream.lib
    ${EPOC_LIB}/charconv.lib
    ${EPOC_LIB}/bitgdi.lib
    ${EPOC_LIB}/euser.lib
    ${EPOC_LIB}/estlib.lib
    ${EPOC_LIB}/ws32.lib
    ${EPOC_LIB}/hal.lib
    ${EPOC_LIB}/fbscli.lib
    ${EPOC_LIB}/efsrv.lib
    ${EPOC_LIB}/scdv.lib
    ${EPOC_LIB}/gdi.lib
  )
endif()
