# SDL 1.2

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SRC_DIR       "${NGAGESDK}/modules/SDL-1.2/src")
set(EAUDIOLIB_DIR "${SRC_DIR}/EAudioLib")
set(SDL12_DIR     "${SRC_DIR}/SDL")
set(SDL12_INC_DIR "${SDL12_DIR}/include")

set(SDL12_sources
    # General sources
    "${SDL12_DIR}/src/SDL.c"
    "${SDL12_DIR}/src/SDL_error.c"
    "${SDL12_DIR}/src/SDL_fatal.c"
    # Video sources
    "${SDL12_DIR}/src/video/SDL_blit.c"
    "${SDL12_DIR}/src/video/SDL_blit_0.c"
    "${SDL12_DIR}/src/video/SDL_blit_1.c"
    "${SDL12_DIR}/src/video/SDL_blit_A.c"
    "${SDL12_DIR}/src/video/SDL_blit_N.c"
    "${SDL12_DIR}/src/video/SDL_bmp.c"
    "${SDL12_DIR}/src/video/SDL_cursor.c"
    "${SDL12_DIR}/src/video/SDL_gamma.c"
    "${SDL12_DIR}/src/video/SDL_pixels.c"
    "${SDL12_DIR}/src/video/SDL_RLEaccel.c"
    "${SDL12_DIR}/src/video/SDL_stretch.c"
    "${SDL12_DIR}/src/video/SDL_surface.c"
    "${SDL12_DIR}/src/video/SDL_video.c"
    "${SDL12_DIR}/src/video/SDL_yuv.c"
    "${SDL12_DIR}/src/video/SDL_yuv_mmx.c"
    "${SDL12_DIR}/src/video/SDL_yuv_sw.c"
    "${SDL12_DIR}/src/video/Epoc/SDL_epocvideo.cpp"
    "${SDL12_DIR}/src/video/Epoc/SDL_epocevents.cpp"
    # Audio sources
    "${SDL12_DIR}/src/audio/SDL_audio.c"
    "${SDL12_DIR}/src/audio/SDL_audiocvt.c"
    "${SDL12_DIR}/src/audio/SDL_audiodev.c"
    "${SDL12_DIR}/src/audio/SDL_audiomem.c"
    "${SDL12_DIR}/src/audio/SDL_mixer.c"
    "${SDL12_DIR}/src/audio/SDL_wave.c"
    "${SDL12_DIR}/src/audio/epoc/SDL_epocaudio.cpp"
    #"${SDL12_DIR}/src/audio/epoc/SDL_epocaudiosync.cpp"
    # Endian sources
    "${SDL12_DIR}/src/endian/SDL_endian.c"
    # Thread sources
    "${SDL12_DIR}/src/thread/SDL_thread.c"
    "${SDL12_DIR}/src/thread/generic/SDL_syscond.c"
    "${SDL12_DIR}/src/thread/epoc/SDL_sysmutex.cpp"
    "${SDL12_DIR}/src/thread/epoc/SDL_syssem.cpp"
    "${SDL12_DIR}/src/thread/epoc/SDL_systhread.cpp"
    # Events sources
    "${SDL12_DIR}/src/events/SDL_active.c"
    "${SDL12_DIR}/src/events/SDL_events.c"
    "${SDL12_DIR}/src/events/SDL_keyboard.c"
    "${SDL12_DIR}/src/events/SDL_mouse.c"
    "${SDL12_DIR}/src/events/SDL_quit.c"
    "${SDL12_DIR}/src/events/SDL_resize.c"
    # Timer sources
    "${SDL12_DIR}/src/timer/SDL_timer.c"
    "${SDL12_DIR}/src/timer/epoc/SDL_systimer.cpp"
    # File sources
    "${SDL12_DIR}/src/file/SDL_rwops.c")

add_library(SDL12 STATIC ${SDL12_sources})

target_compile_definitions(
    SDL12
    PUBLIC
    SYMBIAN_SERIES60
    NO_SIGNAL_H
    ENABLE_EPOC
    DISABLE_JOYSTICK
    DISABLE_CDROM
    ${GCC_MODULE_DEFS})

target_include_directories(
    SDL12
    PUBLIC
    ${SDL12_INC_DIR}
    ${SDL12_DIR}/src/
    ${SDL12_DIR}/src/video
    ${SDL12_DIR}/src/video/epoc
    ${SDL12_DIR}/src/events
    ${SDL12_DIR}/src/audio
    ${SDL12_DIR}/src/audio/epoc
    ${SDL12_DIR}/src/main/epoc
    ${SDL12_DIR}/src/thread
    ${SDL12_DIR}/src/thread/generic
    ${SDL12_DIR}/src/thread/epoc
    ${SDL12_DIR}/src/timer
    ${SDL12_DIR}/src/joystick
    ${SRC_DIR}/SDL_epocruntime/inc
    ${SRC_DIR}/include
    ${EAUDIOLIB_DIR}/inc)
