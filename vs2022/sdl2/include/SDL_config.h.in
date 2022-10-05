/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef SDL_config_h_
#define SDL_config_h_

/**
 *  \file SDL_config.h.in
 *
 *  This is a set of defines to configure the SDL features
 */

/* General platform specific identifiers */
#include "SDL_platform.h"

/* Make sure that this isn't included by Visual C++ */
#ifdef _MSC_VER
#error You should run hg revert SDL_config.h 
#endif

/* C language features */
#undef const
#undef inline
#undef volatile

/* C datatypes */
#ifdef __LP64__
#define SIZEOF_VOIDP 8
#else
#define SIZEOF_VOIDP 4
#endif
#undef HAVE_GCC_ATOMICS
#undef HAVE_GCC_SYNC_LOCK_TEST_AND_SET

#undef HAVE_DDRAW_H
#undef HAVE_DINPUT_H
#undef HAVE_DSOUND_H
#undef HAVE_DXGI_H
#undef HAVE_XINPUT_H
#undef HAVE_XINPUT_GAMEPAD_EX
#undef HAVE_XINPUT_STATE_EX

/* Comment this if you want to build without any C library requirements */
#undef HAVE_LIBC
#if HAVE_LIBC

/* Useful headers */
#undef HAVE_ALLOCA_H
#undef HAVE_SYS_TYPES_H
#undef HAVE_STDIO_H
#undef STDC_HEADERS
#undef HAVE_STDLIB_H
#undef HAVE_STDARG_H
#undef HAVE_MALLOC_H
#undef HAVE_MEMORY_H
#undef HAVE_STRING_H
#undef HAVE_STRINGS_H
#undef HAVE_WCHAR_H
#undef HAVE_INTTYPES_H
#undef HAVE_STDINT_H
#undef HAVE_CTYPE_H
#undef HAVE_MATH_H
#undef HAVE_ICONV_H
#undef HAVE_SIGNAL_H
#undef HAVE_ALTIVEC_H
#undef HAVE_PTHREAD_NP_H
#undef HAVE_LIBUDEV_H
#undef HAVE_DBUS_DBUS_H
#undef HAVE_IBUS_IBUS_H
#undef HAVE_FCITX_FRONTEND_H
#undef HAVE_LIBSAMPLERATE_H

/* C library functions */
#undef HAVE_MALLOC
#undef HAVE_CALLOC
#undef HAVE_REALLOC
#undef HAVE_FREE
#undef HAVE_ALLOCA
#ifndef __WIN32__ /* Don't use C runtime versions of these on Windows */
#undef HAVE_GETENV
#undef HAVE_SETENV
#undef HAVE_PUTENV
#undef HAVE_UNSETENV
#endif
#undef HAVE_QSORT
#undef HAVE_ABS
#undef HAVE_BCOPY
#undef HAVE_MEMSET
#undef HAVE_MEMCPY
#undef HAVE_MEMMOVE
#undef HAVE_MEMCMP
#undef HAVE_WCSLEN
#undef HAVE_WCSLCPY
#undef HAVE_WCSLCAT
#undef HAVE_WCSCMP
#undef HAVE_STRLEN
#undef HAVE_STRLCPY
#undef HAVE_STRLCAT
#undef HAVE_STRDUP
#undef HAVE__STRREV
#undef HAVE__STRUPR
#undef HAVE__STRLWR
#undef HAVE_INDEX
#undef HAVE_RINDEX
#undef HAVE_STRCHR
#undef HAVE_STRRCHR
#undef HAVE_STRSTR
#undef HAVE_ITOA
#undef HAVE__LTOA
#undef HAVE__UITOA
#undef HAVE__ULTOA
#undef HAVE_STRTOL
#undef HAVE_STRTOUL
#undef HAVE__I64TOA
#undef HAVE__UI64TOA
#undef HAVE_STRTOLL
#undef HAVE_STRTOULL
#undef HAVE_STRTOD
#undef HAVE_ATOI
#undef HAVE_ATOF
#undef HAVE_STRCMP
#undef HAVE_STRNCMP
#undef HAVE__STRICMP
#undef HAVE_STRCASECMP
#undef HAVE__STRNICMP
#undef HAVE_STRNCASECMP
#undef HAVE_SSCANF
#undef HAVE_VSSCANF
#undef HAVE_SNPRINTF
#undef HAVE_VSNPRINTF
#undef HAVE_M_PI
#undef HAVE_ATAN
#undef HAVE_ATAN2
#undef HAVE_ACOS
#undef HAVE_ASIN
#undef HAVE_CEIL
#undef HAVE_COPYSIGN
#undef HAVE_COS
#undef HAVE_COSF
#undef HAVE_FABS
#undef HAVE_FLOOR
#undef HAVE_LOG
#undef HAVE_POW
#undef HAVE_SCALBN
#undef HAVE_SIN
#undef HAVE_SINF
#undef HAVE_SQRT
#undef HAVE_SQRTF
#undef HAVE_TAN
#undef HAVE_TANF
#undef HAVE_FOPEN64
#undef HAVE_FSEEKO
#undef HAVE_FSEEKO64
#undef HAVE_SIGACTION
#undef HAVE_SA_SIGACTION
#undef HAVE_SETJMP
#undef HAVE_NANOSLEEP
#undef HAVE_SYSCONF
#undef HAVE_SYSCTLBYNAME
#undef HAVE_CLOCK_GETTIME
#undef HAVE_GETPAGESIZE
#undef HAVE_MPROTECT
#undef HAVE_ICONV
#undef HAVE_PTHREAD_SETNAME_NP
#undef HAVE_PTHREAD_SET_NAME_NP
#undef HAVE_SEM_TIMEDWAIT
#undef HAVE_GETAUXVAL
#undef HAVE_POLL

#else
#define HAVE_STDARG_H   1
#define HAVE_STDDEF_H   1
#define HAVE_STDINT_H   1
#endif /* HAVE_LIBC */

/* SDL internal assertion support */
#undef SDL_DEFAULT_ASSERT_LEVEL

/* Allow disabling of core subsystems */
#undef SDL_ATOMIC_DISABLED
#undef SDL_AUDIO_DISABLED
#undef SDL_CPUINFO_DISABLED
#undef SDL_EVENTS_DISABLED
#undef SDL_FILE_DISABLED
#undef SDL_JOYSTICK_DISABLED
#undef SDL_HAPTIC_DISABLED
#undef SDL_LOADSO_DISABLED
#undef SDL_RENDER_DISABLED
#undef SDL_THREADS_DISABLED
#undef SDL_TIMERS_DISABLED
#undef SDL_VIDEO_DISABLED
#undef SDL_POWER_DISABLED
#undef SDL_FILESYSTEM_DISABLED

/* Enable various audio drivers */
#undef SDL_AUDIO_DRIVER_ALSA
#undef SDL_AUDIO_DRIVER_ALSA_DYNAMIC
#undef SDL_AUDIO_DRIVER_ANDROID
#undef SDL_AUDIO_DRIVER_ARTS
#undef SDL_AUDIO_DRIVER_ARTS_DYNAMIC
#undef SDL_AUDIO_DRIVER_COREAUDIO
#undef SDL_AUDIO_DRIVER_DISK
#undef SDL_AUDIO_DRIVER_DSOUND
#undef SDL_AUDIO_DRIVER_DUMMY
#undef SDL_AUDIO_DRIVER_EMSCRIPTEN
#undef SDL_AUDIO_DRIVER_ESD
#undef SDL_AUDIO_DRIVER_ESD_DYNAMIC
#undef SDL_AUDIO_DRIVER_FUSIONSOUND
#undef SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC
#undef SDL_AUDIO_DRIVER_HAIKU
#undef SDL_AUDIO_DRIVER_JACK
#undef SDL_AUDIO_DRIVER_JACK_DYNAMIC
#undef SDL_AUDIO_DRIVER_NACL
#undef SDL_AUDIO_DRIVER_NAS
#undef SDL_AUDIO_DRIVER_NAS_DYNAMIC
#undef SDL_AUDIO_DRIVER_NETBSD
#undef SDL_AUDIO_DRIVER_OSS
#undef SDL_AUDIO_DRIVER_OSS_SOUNDCARD_H
#undef SDL_AUDIO_DRIVER_PAUDIO
#undef SDL_AUDIO_DRIVER_PULSEAUDIO
#undef SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC
#undef SDL_AUDIO_DRIVER_QSA
#undef SDL_AUDIO_DRIVER_SNDIO
#undef SDL_AUDIO_DRIVER_SNDIO_DYNAMIC
#undef SDL_AUDIO_DRIVER_SUNAUDIO
#undef SDL_AUDIO_DRIVER_WASAPI
#undef SDL_AUDIO_DRIVER_WINMM
#undef SDL_AUDIO_DRIVER_XAUDIO2

/* Enable various input drivers */
#undef SDL_INPUT_LINUXEV
#undef SDL_INPUT_LINUXKD
#undef SDL_INPUT_TSLIB
#undef SDL_JOYSTICK_HAIKU
#undef SDL_JOYSTICK_DINPUT
#undef SDL_JOYSTICK_XINPUT
#undef SDL_JOYSTICK_DUMMY
#undef SDL_JOYSTICK_IOKIT
#undef SDL_JOYSTICK_LINUX
#undef SDL_JOYSTICK_ANDROID
#undef SDL_JOYSTICK_WINMM
#undef SDL_JOYSTICK_USBHID
#undef SDL_JOYSTICK_USBHID_MACHINE_JOYSTICK_H
#undef SDL_JOYSTICK_EMSCRIPTEN
#undef SDL_HAPTIC_DUMMY
#undef SDL_HAPTIC_LINUX
#undef SDL_HAPTIC_IOKIT
#undef SDL_HAPTIC_DINPUT
#undef SDL_HAPTIC_XINPUT

/* Enable various shared object loading systems */
#undef SDL_LOADSO_DLOPEN
#undef SDL_LOADSO_DUMMY
#undef SDL_LOADSO_LDG
#undef SDL_LOADSO_WINDOWS

/* Enable various threading systems */
#undef SDL_THREAD_PTHREAD
#undef SDL_THREAD_PTHREAD_RECURSIVE_MUTEX
#undef SDL_THREAD_PTHREAD_RECURSIVE_MUTEX_NP
#undef SDL_THREAD_WINDOWS

/* Enable various timer systems */
#undef SDL_TIMER_HAIKU
#undef SDL_TIMER_DUMMY
#undef SDL_TIMER_UNIX
#undef SDL_TIMER_WINDOWS

/* Enable various video drivers */
#undef SDL_VIDEO_DRIVER_HAIKU
#undef SDL_VIDEO_DRIVER_COCOA
#undef SDL_VIDEO_DRIVER_DIRECTFB
#undef SDL_VIDEO_DRIVER_DIRECTFB_DYNAMIC
#undef SDL_VIDEO_DRIVER_DUMMY
#undef SDL_VIDEO_DRIVER_WINDOWS
#undef SDL_VIDEO_DRIVER_WAYLAND
#undef SDL_VIDEO_DRIVER_WAYLAND_QT_TOUCH
#undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC
#undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL
#undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR
#undef SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON
#undef SDL_VIDEO_DRIVER_MIR
#undef SDL_VIDEO_DRIVER_MIR_DYNAMIC
#undef SDL_VIDEO_DRIVER_MIR_DYNAMIC_XKBCOMMON
#undef SDL_VIDEO_DRIVER_X11
#undef SDL_VIDEO_DRIVER_RPI
#undef SDL_VIDEO_DRIVER_KMSDRM
#undef SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC
#undef SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC_GBM
#undef SDL_VIDEO_DRIVER_ANDROID
#undef SDL_VIDEO_DRIVER_EMSCRIPTEN
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XINERAMA
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT2
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS
#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XVIDMODE
#undef SDL_VIDEO_DRIVER_X11_XCURSOR
#undef SDL_VIDEO_DRIVER_X11_XDBE
#undef SDL_VIDEO_DRIVER_X11_XINERAMA
#undef SDL_VIDEO_DRIVER_X11_XINPUT2
#undef SDL_VIDEO_DRIVER_X11_XINPUT2_SUPPORTS_MULTITOUCH
#undef SDL_VIDEO_DRIVER_X11_XRANDR
#undef SDL_VIDEO_DRIVER_X11_XSCRNSAVER
#undef SDL_VIDEO_DRIVER_X11_XSHAPE
#undef SDL_VIDEO_DRIVER_X11_XVIDMODE
#undef SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS
#undef SDL_VIDEO_DRIVER_X11_CONST_PARAM_XEXTADDDISPLAY
#undef SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM
#undef SDL_VIDEO_DRIVER_NACL
#undef SDL_VIDEO_DRIVER_VIVANTE
#undef SDL_VIDEO_DRIVER_VIVANTE_VDK
#undef SDL_VIDEO_DRIVER_QNX

#undef SDL_VIDEO_RENDER_D3D
#undef SDL_VIDEO_RENDER_D3D11
#undef SDL_VIDEO_RENDER_OGL
#undef SDL_VIDEO_RENDER_OGL_ES
#undef SDL_VIDEO_RENDER_OGL_ES2
#undef SDL_VIDEO_RENDER_DIRECTFB

/* Enable OpenGL support */
#undef SDL_VIDEO_OPENGL
#undef SDL_VIDEO_OPENGL_ES
#undef SDL_VIDEO_OPENGL_ES2
#undef SDL_VIDEO_OPENGL_BGL
#undef SDL_VIDEO_OPENGL_CGL
#undef SDL_VIDEO_OPENGL_EGL
#undef SDL_VIDEO_OPENGL_GLX
#undef SDL_VIDEO_OPENGL_WGL
#undef SDL_VIDEO_OPENGL_OSMESA
#undef SDL_VIDEO_OPENGL_OSMESA_DYNAMIC

/* Enable Vulkan support */
#undef SDL_VIDEO_VULKAN

/* Enable system power support */
#undef SDL_POWER_LINUX
#undef SDL_POWER_WINDOWS
#undef SDL_POWER_MACOSX
#undef SDL_POWER_HAIKU
#undef SDL_POWER_ANDROID
#undef SDL_POWER_EMSCRIPTEN
#undef SDL_POWER_HARDWIRED

/* Enable system filesystem support */
#undef SDL_FILESYSTEM_HAIKU
#undef SDL_FILESYSTEM_COCOA
#undef SDL_FILESYSTEM_DUMMY
#undef SDL_FILESYSTEM_UNIX
#undef SDL_FILESYSTEM_WINDOWS
#undef SDL_FILESYSTEM_NACL
#undef SDL_FILESYSTEM_ANDROID
#undef SDL_FILESYSTEM_EMSCRIPTEN

/* Enable assembly routines */
#undef SDL_ASSEMBLY_ROUTINES
#undef SDL_ALTIVEC_BLITTERS

/* Enable ime support */
#undef SDL_USE_IME

/* Enable dynamic udev support */
#undef SDL_UDEV_DYNAMIC

/* Enable dynamic libsamplerate support */
#undef SDL_LIBSAMPLERATE_DYNAMIC

#endif /* SDL_config_h_ */
