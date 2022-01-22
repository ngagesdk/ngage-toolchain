# EAudioLib

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SRC_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/../../projects/SDL-1.2/src")
set(SDL_DIR       "${SRC_DIR}/SDL")
set(EAUDIOLIB_DIR "${SRC_DIR}/EAudioLib")

set(EAudioLib_sources
    "${EAUDIOLIB_DIR}/src/audiolib.cpp"
    "${EAUDIOLIB_DIR}/src/audioplayer.cpp"
    "${EAUDIOLIB_DIR}/src/tinysdl.cpp"
    "${EAUDIOLIB_DIR}/src/audioconvert.cpp"
    # SDL
    "${EAUDIOLIB_DIR}/Scumm/backends/sdl/sdl.cpp"
    "${EAUDIOLIB_DIR}/Scumm/backends/sdl/sdl-common.cpp"
    "${EAUDIOLIB_DIR}/Scumm/backends/sdl/sdl-symbian.cpp"
    # AUDIO
    "${EAUDIOLIB_DIR}/Scumm/sound/audiostream.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/fmopl.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/mididrv.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/midiparser.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/midiparser_smf.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/midiparser_xmidi.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/mixer.cpp"
    #"${EAUDIOLIB_DIR}/Scumm/sound/mpu401.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/rate.cpp"
    #"${EAUDIOLIB_DIR}/Scumm/sound/resample.cpp"
    #"${EAUDIOLIB_DIR}/Scumm/sound/voc.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/midiparser_ro.cpp"
    "${EAUDIOLIB_DIR}/Scumm/sound/midiparser_eup.cpp"
    # ADLIB
    "${EAUDIOLIB_DIR}/Scumm/backends/midi/adlib.cpp"
    # UTILS
    "${EAUDIOLIB_DIR}/Scumm/common/util.cpp"
    "${EAUDIOLIB_DIR}/Scumm/common/timer.cpp"
    "${EAUDIOLIB_DIR}/Scumm/common/str.cpp"
    "${EAUDIOLIB_DIR}/Scumm/common/file.cpp"
    "${EAUDIOLIB_DIR}/Scumm/common/system.cpp")

add_library(EAudioLib STATIC ${EAudioLib_sources})

target_compile_definitions(
    EAudioLib
    PUBLIC
    ${GCC_MODULE_DEFS})

target_include_directories(
    EAudioLib
    PUBLIC
    ${EAUDIOLIB_DIR}/inc
    ${EAUDIOLIB_DIR}/Scumm
    ${EAUDIOLIB_DIR}/Scumm/common)
