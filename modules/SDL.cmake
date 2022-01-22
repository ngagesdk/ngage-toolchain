# SDL 1.2

set(UID1_SDL 0x1000007a) # KExecutableImageUidValue, e32uid.h
set(UID2_SDL 0x100039ce) # KAppUidValue16, apadef.h
set(UID3_SDL 0x1053444c) # SDL

set(GCC_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__ __EXE__)
set(GCC_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_USER_DEFS -DUID1=${UID1} -DUID2=${UID2} -DUID3=${UID3})
set(GCC_DEFS      ${GCC_COMN_DEFS} ${GCC_MODE_DEFS} ${GCC_USER_DEFS})

set(SRC_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/../../projects/SDL-1.2/src")
set(SDL_DIR       "${SRC_DIR}/SDL")
set(EAUDIOLIB_DIR "${SRC_DIR}/EAudioLib")

set(SDL_sources
    # General sources
    "${SDL_DIR}/src/SDL.c"
    "${SDL_DIR}/src/SDL_error.c"
    "${SDL_DIR}/src/SDL_fatal.c"
    # Video sources
    "${SDL_DIR}/src/video/SDL_blit.c"
    "${SDL_DIR}/src/video/SDL_blit_0.c"
    "${SDL_DIR}/src/video/SDL_blit_1.c"
    "${SDL_DIR}/src/video/SDL_blit_A.c"
    "${SDL_DIR}/src/video/SDL_blit_N.c"
    "${SDL_DIR}/src/video/SDL_bmp.c"
    "${SDL_DIR}/src/video/SDL_cursor.c"
    "${SDL_DIR}/src/video/SDL_gamma.c"
    "${SDL_DIR}/src/video/SDL_pixels.c"
    "${SDL_DIR}/src/video/SDL_RLEaccel.c"
    "${SDL_DIR}/src/video/SDL_stretch.c"
    "${SDL_DIR}/src/video/SDL_surface.c"
    "${SDL_DIR}/src/video/SDL_video.c"
    "${SDL_DIR}/src/video/SDL_yuv.c"
    "${SDL_DIR}/src/video/SDL_yuv_mmx.c"
    "${SDL_DIR}/src/video/SDL_yuv_sw.c"
    "${SDL_DIR}/src/video/Epoc/SDL_epocvideo.cpp"
    "${SDL_DIR}/src/video/Epoc/SDL_epocevents.cpp"
    # Audio sources
    "${SDL_DIR}/src/audio/SDL_audio.c"
    "${SDL_DIR}/src/audio/SDL_audiocvt.c"
    "${SDL_DIR}/src/audio/SDL_audiodev.c"
    "${SDL_DIR}/src/audio/SDL_audiomem.c"
    "${SDL_DIR}/src/audio/SDL_mixer.c"
    "${SDL_DIR}/src/audio/SDL_wave.c"
    "${SDL_DIR}/src/audio/epoc/SDL_epocaudio.cpp"
    #"${SDL_DIR}/src/audio/epoc/SDL_epocaudiosync.cpp"
    # Endian sources
    "${SDL_DIR}/src/endian/SDL_endian.c"
    # Thread sources
    "${SDL_DIR}/src/thread/SDL_thread.c"
    "${SDL_DIR}/src/thread/generic/SDL_syscond.c"
    "${SDL_DIR}/src/thread/epoc/SDL_sysmutex.cpp"
    "${SDL_DIR}/src/thread/epoc/SDL_syssem.cpp"
    "${SDL_DIR}/src/thread/epoc/SDL_systhread.cpp"
    # Events sources
    "${SDL_DIR}/src/events/SDL_active.c"
    "${SDL_DIR}/src/events/SDL_events.c"
    "${SDL_DIR}/src/events/SDL_keyboard.c"
    "${SDL_DIR}/src/events/SDL_mouse.c"
    "${SDL_DIR}/src/events/SDL_quit.c"
    "${SDL_DIR}/src/events/SDL_resize.c"
    # Timer sources
    "${SDL_DIR}/src/timer/SDL_timer.c"
    "${SDL_DIR}/src/timer/epoc/SDL_systimer.cpp"
    # File sources
    "${SDL_DIR}/src/file/SDL_rwops.c")

add_library(SDL STATIC ${SDL_sources})

target_compile_definitions(
    SDL
    PUBLIC
    SYMBIAN_SERIES60
    NO_SIGNAL_H
    ENABLE_EPOC
    DISABLE_JOYSTICK
    DISABLE_CDROM
    ${GCC_DEFS})

target_include_directories(
    SDL
    PUBLIC
    ${SDL_DIR}/include
    ${SDL_DIR}/src/
    ${SDL_DIR}/src/video
    ${SDL_DIR}/src/video/epoc
    ${SDL_DIR}/src/events
    ${SDL_DIR}/src/audio
    ${SDL_DIR}/src/audio/epoc
    ${SDL_DIR}/src/main/epoc
    ${SDL_DIR}/src/thread
    ${SDL_DIR}/src/thread/generic
    ${SDL_DIR}/src/thread/epoc
    ${SDL_DIR}/src/timer
    ${SDL_DIR}/src/joystick
    ${SRC_DIR}/SDL_epocruntime/inc
    ${SRC_DIR}/include
    ${EAUDIOLIB_DIR}/inc)
