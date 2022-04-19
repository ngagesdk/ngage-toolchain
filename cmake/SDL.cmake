# SDL 2

include_guard(GLOBAL)

set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

set(SDL_DIR     "${NGAGESDK}/modules/SDL/")
set(SRC_DIR     "${SDL_DIR}/src")
set(SDL_INC_DIR "${SDL_DIR}/include")

set(SDL_sources
    "${SRC_DIR}/SDL_assert.c"
    "${SRC_DIR}/SDL.c"
    "${SRC_DIR}/SDL_dataqueue.c"
    "${SRC_DIR}/SDL_error.c"
    "${SRC_DIR}/SDL_hints.c"
    "${SRC_DIR}/SDL_list.c"
    "${SRC_DIR}/SDL_log.c"
    "${SRC_DIR}/atomic/SDL_atomic.c"
    "${SRC_DIR}/atomic/SDL_spinlock.c"
    "${SRC_DIR}/audio/SDL_audio.c"
    "${SRC_DIR}/audio/SDL_audiocvt.c"
    "${SRC_DIR}/audio/SDL_audiodev.c"
    "${SRC_DIR}/audio/SDL_audiotypecvt.c"
    "${SRC_DIR}/audio/SDL_mixer.c"
    "${SRC_DIR}/audio/SDL_wave.c"
    "${SRC_DIR}/audio/dummy/SDL_dummyaudio.c"
    "${SRC_DIR}/cpuinfo/SDL_cpuinfo.c"
    "${SRC_DIR}/dynapi/SDL_dynapi.c"
    "${SRC_DIR}/events/imKStoUCS.c"
    "${SRC_DIR}/events/SDL_clipboardevents.c"
    "${SRC_DIR}/events/SDL_displayevents.c"
    "${SRC_DIR}/events/SDL_dropevents.c"
    "${SRC_DIR}/events/SDL_events.c"
    "${SRC_DIR}/events/SDL_gesture.c"
    "${SRC_DIR}/events/SDL_keyboard.c"
    "${SRC_DIR}/events/SDL_mouse.c"
    "${SRC_DIR}/events/SDL_quit.c"
    "${SRC_DIR}/events/SDL_touch.c"
    "${SRC_DIR}/events/SDL_windowevents.c"
    "${SRC_DIR}/file/SDL_rwops.c"
    "${SRC_DIR}/filesystem/dummy/SDL_sysfilesystem.c"
    "${SRC_DIR}/haptic/SDL_haptic.c"
    "${SRC_DIR}/haptic/dummy/SDL_syshaptic.c"
    "${SRC_DIR}/hidapi/SDL_hidapi.c"
    "${SRC_DIR}/joystick/SDL_gamecontroller.c"
    "${SRC_DIR}/joystick/SDL_joystick.c"
    "${SRC_DIR}/joystick/dummy/SDL_sysjoystick.c"
    "${SRC_DIR}/libm/e_atan2.c"
    "${SRC_DIR}/libm/e_exp.c"
    "${SRC_DIR}/libm/e_fmod.c"
    "${SRC_DIR}/libm/e_log10.c"
    "${SRC_DIR}/libm/e_log.c"
    "${SRC_DIR}/libm/e_pow.c"
    "${SRC_DIR}/libm/e_rem_pio2.c"
    "${SRC_DIR}/libm/e_sqrt.c"
    "${SRC_DIR}/libm/k_cos.c"
    "${SRC_DIR}/libm/k_rem_pio2.c"
    "${SRC_DIR}/libm/k_sin.c"
    "${SRC_DIR}/libm/k_tan.c"
    "${SRC_DIR}/libm/s_atan.c"
    "${SRC_DIR}/libm/s_copysign.c"
    "${SRC_DIR}/libm/s_cos.c"
    "${SRC_DIR}/libm/s_fabs.c"
    "${SRC_DIR}/libm/s_floor.c"
    "${SRC_DIR}/libm/s_scalbn.c"
    "${SRC_DIR}/libm/s_sin.c"
    "${SRC_DIR}/libm/s_tan.c"
    "${SRC_DIR}/loadso/dummy/SDL_sysloadso.c"
    "${SRC_DIR}/locale/SDL_locale.c"
    "${SRC_DIR}/locale/dummy/SDL_syslocale.c"
    "${SRC_DIR}/main/ngage/SDL_ngage_main.cpp"
    "${SRC_DIR}/misc/SDL_url.c"
    "${SRC_DIR}/misc/dummy/SDL_sysurl.c"
    "${SRC_DIR}/power/SDL_power.c"
    "${SRC_DIR}/render/SDL_d3dmath.c"
    "${SRC_DIR}/render/SDL_render.c"
    "${SRC_DIR}/render/SDL_yuv_sw.c"
    "${SRC_DIR}/render/software/SDL_blendfillrect.c"
    "${SRC_DIR}/render/software/SDL_blendline.c"
    "${SRC_DIR}/render/software/SDL_blendpoint.c"
    "${SRC_DIR}/render/software/SDL_drawline.c"
    "${SRC_DIR}/render/software/SDL_drawpoint.c"
    "${SRC_DIR}/render/software/SDL_render_sw.c"
    "${SRC_DIR}/render/software/SDL_rotate.c"
    "${SRC_DIR}/render/software/SDL_triangle.c"
    "${SRC_DIR}/sensor/SDL_sensor.c"
    "${SRC_DIR}/sensor/dummy/SDL_dummysensor.c"
    "${SRC_DIR}/stdlib/SDL_crc32.c"
    "${SRC_DIR}/stdlib/SDL_getenv.c"
    "${SRC_DIR}/stdlib/SDL_iconv.c"
    "${SRC_DIR}/stdlib/SDL_malloc.c"
    "${SRC_DIR}/stdlib/SDL_qsort.c"
    "${SRC_DIR}/stdlib/SDL_stdlib.c"
    "${SRC_DIR}/stdlib/SDL_string.c"
    "${SRC_DIR}/stdlib/SDL_strtokr.c"
    "${SRC_DIR}/thread/SDL_thread.c"
    "${SRC_DIR}/thread/generic/SDL_systls.c"
    "${SRC_DIR}/thread/ngage/SDL_sysmutex.cpp"
    "${SRC_DIR}/thread/ngage/SDL_syssem.cpp"
    "${SRC_DIR}/thread/ngage/SDL_systhread.cpp"
    "${SRC_DIR}/timer/SDL_timer.c"
    "${SRC_DIR}/timer/ngage/SDL_systimer.cpp"
    "${SRC_DIR}/video/SDL_blit_0.c"
    "${SRC_DIR}/video/SDL_blit_1.c"
    "${SRC_DIR}/video/SDL_blit_A.c"
    "${SRC_DIR}/video/SDL_blit_auto.c"
    "${SRC_DIR}/video/SDL_blit.c"
    "${SRC_DIR}/video/SDL_blit_copy.c"
    "${SRC_DIR}/video/SDL_blit_N.c"
    "${SRC_DIR}/video/SDL_blit_slow.c"
    "${SRC_DIR}/video/SDL_bmp.c"
    "${SRC_DIR}/video/SDL_clipboard.c"
    "${SRC_DIR}/video/SDL_egl.c"
    "${SRC_DIR}/video/SDL_fillrect.c"
    "${SRC_DIR}/video/SDL_pixels.c"
    "${SRC_DIR}/video/SDL_rect.c"
    "${SRC_DIR}/video/SDL_RLEaccel.c"
    "${SRC_DIR}/video/SDL_shape.c"
    "${SRC_DIR}/video/SDL_stretch.c"
    "${SRC_DIR}/video/SDL_surface.c"
    "${SRC_DIR}/video/SDL_video.c"
    "${SRC_DIR}/video/SDL_vulkan_utils.c"
    "${SRC_DIR}/video/SDL_yuv.c"
    "${SRC_DIR}/video/ngage/SDL_ngageevents.cpp"
    "${SRC_DIR}/video/ngage/SDL_ngageframebuffer.cpp"
    "${SRC_DIR}/video/ngage/SDL_ngagevideo.cpp"
    "${SRC_DIR}/video/ngage/SDL_ngagewindow.cpp"
    "${SRC_DIR}/video/yuv2rgb/yuv_rgb.c")

add_library(SDL STATIC ${SDL_sources})

target_compile_definitions(
    SDL
    PUBLIC
    SDL_STATIC_LIB
    ${GCC_MODULE_DEFS})

target_compile_options(
    SDL
    PUBLIC
    -O3)

target_include_directories(
    SDL
    PUBLIC
    ${SDL_INC_DIR})
