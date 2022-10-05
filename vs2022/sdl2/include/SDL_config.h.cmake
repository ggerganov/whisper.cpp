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

/* C language features */
#cmakedefine const @HAVE_CONST@
#cmakedefine inline @HAVE_INLINE@
#cmakedefine volatile @HAVE_VOLATILE@

/* C datatypes */
/* Define SIZEOF_VOIDP for 64/32 architectures */
#ifdef __LP64__
#define SIZEOF_VOIDP 8
#else
#define SIZEOF_VOIDP 4
#endif

#cmakedefine HAVE_GCC_ATOMICS @HAVE_GCC_ATOMICS@
#cmakedefine HAVE_GCC_SYNC_LOCK_TEST_AND_SET @HAVE_GCC_SYNC_LOCK_TEST_AND_SET@

#cmakedefine HAVE_D3D_H @HAVE_D3D_H@
#cmakedefine HAVE_D3D11_H @HAVE_D3D11_H@
#cmakedefine HAVE_DDRAW_H @HAVE_DDRAW_H@
#cmakedefine HAVE_DSOUND_H @HAVE_DSOUND_H@
#cmakedefine HAVE_DINPUT_H @HAVE_DINPUT_H@
#cmakedefine HAVE_XAUDIO2_H @HAVE_XAUDIO2_H@
#cmakedefine HAVE_XINPUT_H @HAVE_XINPUT_H@
#cmakedefine HAVE_DXGI_H @HAVE_DXGI_H@
#cmakedefine HAVE_XINPUT_GAMEPAD_EX @HAVE_XINPUT_GAMEPAD_EX@
#cmakedefine HAVE_XINPUT_STATE_EX @HAVE_XINPUT_STATE_EX@

/* Comment this if you want to build without any C library requirements */
#cmakedefine HAVE_LIBC 1
#if HAVE_LIBC

/* Useful headers */
#cmakedefine HAVE_ALLOCA_H 1
#cmakedefine HAVE_SYS_TYPES_H 1
#cmakedefine HAVE_STDIO_H 1
#cmakedefine STDC_HEADERS 1
#cmakedefine HAVE_STDLIB_H 1
#cmakedefine HAVE_STDARG_H 1
#cmakedefine HAVE_MALLOC_H 1
#cmakedefine HAVE_MEMORY_H 1
#cmakedefine HAVE_STRING_H 1
#cmakedefine HAVE_STRINGS_H 1
#cmakedefine HAVE_WCHAR_H 1
#cmakedefine HAVE_INTTYPES_H 1
#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_CTYPE_H 1
#cmakedefine HAVE_MATH_H 1
#cmakedefine HAVE_ICONV_H 1
#cmakedefine HAVE_SIGNAL_H 1
#cmakedefine HAVE_ALTIVEC_H 1
#cmakedefine HAVE_PTHREAD_NP_H 1
#cmakedefine HAVE_LIBUDEV_H 1
#cmakedefine HAVE_DBUS_DBUS_H 1
#cmakedefine HAVE_IBUS_IBUS_H 1
#cmakedefine HAVE_FCITX_FRONTEND_H 1
#cmakedefine HAVE_LIBSAMPLERATE_H 1

/* C library functions */
#cmakedefine HAVE_MALLOC 1
#cmakedefine HAVE_CALLOC 1
#cmakedefine HAVE_REALLOC 1
#cmakedefine HAVE_FREE 1
#cmakedefine HAVE_ALLOCA 1
#ifndef __WIN32__ /* Don't use C runtime versions of these on Windows */
#cmakedefine HAVE_GETENV 1
#cmakedefine HAVE_SETENV 1
#cmakedefine HAVE_PUTENV 1
#cmakedefine HAVE_UNSETENV 1
#endif
#cmakedefine HAVE_QSORT 1
#cmakedefine HAVE_ABS 1
#cmakedefine HAVE_BCOPY 1
#cmakedefine HAVE_MEMSET 1
#cmakedefine HAVE_MEMCPY 1
#cmakedefine HAVE_MEMMOVE 1
#cmakedefine HAVE_MEMCMP 1
#cmakedefine HAVE_WCSLEN 1
#cmakedefine HAVE_WCSLCPY 1
#cmakedefine HAVE_WCSLCAT 1
#cmakedefine HAVE_WCSCMP 1
#cmakedefine HAVE_STRLEN 1
#cmakedefine HAVE_STRLCPY 1
#cmakedefine HAVE_STRLCAT 1
#cmakedefine HAVE_STRDUP 1
#cmakedefine HAVE__STRREV 1
#cmakedefine HAVE__STRUPR 1
#cmakedefine HAVE__STRLWR 1
#cmakedefine HAVE_INDEX 1
#cmakedefine HAVE_RINDEX 1
#cmakedefine HAVE_STRCHR 1
#cmakedefine HAVE_STRRCHR 1
#cmakedefine HAVE_STRSTR 1
#cmakedefine HAVE_ITOA 1
#cmakedefine HAVE__LTOA 1
#cmakedefine HAVE__UITOA 1
#cmakedefine HAVE__ULTOA 1
#cmakedefine HAVE_STRTOL 1
#cmakedefine HAVE_STRTOUL 1
#cmakedefine HAVE__I64TOA 1
#cmakedefine HAVE__UI64TOA 1
#cmakedefine HAVE_STRTOLL 1
#cmakedefine HAVE_STRTOULL 1
#cmakedefine HAVE_STRTOD 1
#cmakedefine HAVE_ATOI 1
#cmakedefine HAVE_ATOF 1
#cmakedefine HAVE_STRCMP 1
#cmakedefine HAVE_STRNCMP 1
#cmakedefine HAVE__STRICMP 1
#cmakedefine HAVE_STRCASECMP 1
#cmakedefine HAVE__STRNICMP 1
#cmakedefine HAVE_STRNCASECMP 1
#cmakedefine HAVE_VSSCANF 1
#cmakedefine HAVE_VSNPRINTF 1
#cmakedefine HAVE_M_PI 1
#cmakedefine HAVE_ATAN 1
#cmakedefine HAVE_ATAN2 1
#cmakedefine HAVE_ACOS 1
#cmakedefine HAVE_ASIN 1
#cmakedefine HAVE_CEIL 1
#cmakedefine HAVE_COPYSIGN 1
#cmakedefine HAVE_COS 1
#cmakedefine HAVE_COSF 1
#cmakedefine HAVE_FABS 1
#cmakedefine HAVE_FLOOR 1
#cmakedefine HAVE_LOG 1
#cmakedefine HAVE_POW 1
#cmakedefine HAVE_SCALBN 1
#cmakedefine HAVE_SIN 1
#cmakedefine HAVE_SINF 1
#cmakedefine HAVE_SQRT 1
#cmakedefine HAVE_SQRTF 1
#cmakedefine HAVE_TAN 1
#cmakedefine HAVE_TANF 1
#cmakedefine HAVE_FOPEN64 1
#cmakedefine HAVE_FSEEKO 1
#cmakedefine HAVE_FSEEKO64 1
#cmakedefine HAVE_SIGACTION 1
#cmakedefine HAVE_SA_SIGACTION 1
#cmakedefine HAVE_SETJMP 1
#cmakedefine HAVE_NANOSLEEP 1
#cmakedefine HAVE_SYSCONF 1
#cmakedefine HAVE_SYSCTLBYNAME 1
#cmakedefine HAVE_CLOCK_GETTIME 1
#cmakedefine HAVE_GETPAGESIZE 1
#cmakedefine HAVE_MPROTECT 1
#cmakedefine HAVE_ICONV 1
#cmakedefine HAVE_PTHREAD_SETNAME_NP 1
#cmakedefine HAVE_PTHREAD_SET_NAME_NP 1
#cmakedefine HAVE_SEM_TIMEDWAIT 1
#cmakedefine HAVE_GETAUXVAL 1
#cmakedefine HAVE_POLL 1

#elif __WIN32__
#cmakedefine HAVE_STDARG_H 1
#cmakedefine HAVE_STDDEF_H 1
#else
/* We may need some replacement for stdarg.h here */
#include <stdarg.h>
#endif /* HAVE_LIBC */

/* SDL internal assertion support */
#cmakedefine SDL_DEFAULT_ASSERT_LEVEL @SDL_DEFAULT_ASSERT_LEVEL@

/* Allow disabling of core subsystems */
#cmakedefine SDL_ATOMIC_DISABLED @SDL_ATOMIC_DISABLED@
#cmakedefine SDL_AUDIO_DISABLED @SDL_AUDIO_DISABLED@
#cmakedefine SDL_CPUINFO_DISABLED @SDL_CPUINFO_DISABLED@
#cmakedefine SDL_EVENTS_DISABLED @SDL_EVENTS_DISABLED@
#cmakedefine SDL_FILE_DISABLED @SDL_FILE_DISABLED@
#cmakedefine SDL_JOYSTICK_DISABLED @SDL_JOYSTICK_DISABLED@
#cmakedefine SDL_HAPTIC_DISABLED @SDL_HAPTIC_DISABLED@
#cmakedefine SDL_LOADSO_DISABLED @SDL_LOADSO_DISABLED@
#cmakedefine SDL_RENDER_DISABLED @SDL_RENDER_DISABLED@
#cmakedefine SDL_THREADS_DISABLED @SDL_THREADS_DISABLED@
#cmakedefine SDL_TIMERS_DISABLED @SDL_TIMERS_DISABLED@
#cmakedefine SDL_VIDEO_DISABLED @SDL_VIDEO_DISABLED@
#cmakedefine SDL_POWER_DISABLED @SDL_POWER_DISABLED@
#cmakedefine SDL_FILESYSTEM_DISABLED @SDL_FILESYSTEM_DISABLED@

/* Enable various audio drivers */
#cmakedefine SDL_AUDIO_DRIVER_ALSA @SDL_AUDIO_DRIVER_ALSA@
#cmakedefine SDL_AUDIO_DRIVER_ALSA_DYNAMIC @SDL_AUDIO_DRIVER_ALSA_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_ANDROID @SDL_AUDIO_DRIVER_ANDROID@
#cmakedefine SDL_AUDIO_DRIVER_ARTS @SDL_AUDIO_DRIVER_ARTS@
#cmakedefine SDL_AUDIO_DRIVER_ARTS_DYNAMIC @SDL_AUDIO_DRIVER_ARTS_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_COREAUDIO @SDL_AUDIO_DRIVER_COREAUDIO@
#cmakedefine SDL_AUDIO_DRIVER_DISK @SDL_AUDIO_DRIVER_DISK@
#cmakedefine SDL_AUDIO_DRIVER_DSOUND @SDL_AUDIO_DRIVER_DSOUND@
#cmakedefine SDL_AUDIO_DRIVER_DUMMY @SDL_AUDIO_DRIVER_DUMMY@
#cmakedefine SDL_AUDIO_DRIVER_EMSCRIPTEN @SDL_AUDIO_DRIVER_EMSCRIPTEN@
#cmakedefine SDL_AUDIO_DRIVER_ESD @SDL_AUDIO_DRIVER_ESD@
#cmakedefine SDL_AUDIO_DRIVER_ESD_DYNAMIC @SDL_AUDIO_DRIVER_ESD_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_FUSIONSOUND @SDL_AUDIO_DRIVER_FUSIONSOUND@
#cmakedefine SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC @SDL_AUDIO_DRIVER_FUSIONSOUND_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_HAIKU @SDL_AUDIO_DRIVER_HAIKU@
#cmakedefine SDL_AUDIO_DRIVER_JACK @SDL_AUDIO_DRIVER_JACK@
#cmakedefine SDL_AUDIO_DRIVER_JACK_DYNAMIC @SDL_AUDIO_DRIVER_JACK_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_NAS @SDL_AUDIO_DRIVER_NAS@
#cmakedefine SDL_AUDIO_DRIVER_NAS_DYNAMIC @SDL_AUDIO_DRIVER_NAS_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_NETBSD @SDL_AUDIO_DRIVER_NETBSD@
#cmakedefine SDL_AUDIO_DRIVER_OSS @SDL_AUDIO_DRIVER_OSS@
#cmakedefine SDL_AUDIO_DRIVER_OSS_SOUNDCARD_H @SDL_AUDIO_DRIVER_OSS_SOUNDCARD_H@
#cmakedefine SDL_AUDIO_DRIVER_PAUDIO @SDL_AUDIO_DRIVER_PAUDIO@
#cmakedefine SDL_AUDIO_DRIVER_PULSEAUDIO @SDL_AUDIO_DRIVER_PULSEAUDIO@
#cmakedefine SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC @SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_QSA @SDL_AUDIO_DRIVER_QSA@
#cmakedefine SDL_AUDIO_DRIVER_SNDIO @SDL_AUDIO_DRIVER_SNDIO@
#cmakedefine SDL_AUDIO_DRIVER_SNDIO_DYNAMIC @SDL_AUDIO_DRIVER_SNDIO_DYNAMIC@
#cmakedefine SDL_AUDIO_DRIVER_SUNAUDIO @SDL_AUDIO_DRIVER_SUNAUDIO@
#cmakedefine SDL_AUDIO_DRIVER_WASAPI @SDL_AUDIO_DRIVER_WASAPI@
#cmakedefine SDL_AUDIO_DRIVER_WINMM @SDL_AUDIO_DRIVER_WINMM@
#cmakedefine SDL_AUDIO_DRIVER_XAUDIO2 @SDL_AUDIO_DRIVER_XAUDIO2@

/* Enable various input drivers */
#cmakedefine SDL_INPUT_LINUXEV @SDL_INPUT_LINUXEV@
#cmakedefine SDL_INPUT_LINUXKD @SDL_INPUT_LINUXKD@
#cmakedefine SDL_INPUT_TSLIB @SDL_INPUT_TSLIB@
#cmakedefine SDL_JOYSTICK_ANDROID @SDL_JOYSTICK_ANDROID@
#cmakedefine SDL_JOYSTICK_HAIKU @SDL_JOYSTICK_HAIKU@
#cmakedefine SDL_JOYSTICK_DINPUT @SDL_JOYSTICK_DINPUT@
#cmakedefine SDL_JOYSTICK_XINPUT @SDL_JOYSTICK_XINPUT@
#cmakedefine SDL_JOYSTICK_DUMMY @SDL_JOYSTICK_DUMMY@
#cmakedefine SDL_JOYSTICK_IOKIT @SDL_JOYSTICK_IOKIT@
#cmakedefine SDL_JOYSTICK_MFI @SDL_JOYSTICK_MFI@
#cmakedefine SDL_JOYSTICK_LINUX @SDL_JOYSTICK_LINUX@
#cmakedefine SDL_JOYSTICK_WINMM @SDL_JOYSTICK_WINMM@
#cmakedefine SDL_JOYSTICK_USBHID @SDL_JOYSTICK_USBHID@
#cmakedefine SDL_JOYSTICK_USBHID_MACHINE_JOYSTICK_H @SDL_JOYSTICK_USBHID_MACHINE_JOYSTICK_H@
#cmakedefine SDL_JOYSTICK_EMSCRIPTEN @SDL_JOYSTICK_EMSCRIPTEN@
#cmakedefine SDL_HAPTIC_DUMMY @SDL_HAPTIC_DUMMY@
#cmakedefine SDL_HAPTIC_LINUX @SDL_HAPTIC_LINUX@
#cmakedefine SDL_HAPTIC_IOKIT @SDL_HAPTIC_IOKIT@
#cmakedefine SDL_HAPTIC_DINPUT @SDL_HAPTIC_DINPUT@
#cmakedefine SDL_HAPTIC_XINPUT @SDL_HAPTIC_XINPUT@
#cmakedefine SDL_HAPTIC_ANDROID @SDL_HAPTIC_ANDROID@

/* Enable various shared object loading systems */
#cmakedefine SDL_LOADSO_DLOPEN @SDL_LOADSO_DLOPEN@
#cmakedefine SDL_LOADSO_DUMMY @SDL_LOADSO_DUMMY@
#cmakedefine SDL_LOADSO_LDG @SDL_LOADSO_LDG@
#cmakedefine SDL_LOADSO_WINDOWS @SDL_LOADSO_WINDOWS@

/* Enable various threading systems */
#cmakedefine SDL_THREAD_PTHREAD @SDL_THREAD_PTHREAD@
#cmakedefine SDL_THREAD_PTHREAD_RECURSIVE_MUTEX @SDL_THREAD_PTHREAD_RECURSIVE_MUTEX@
#cmakedefine SDL_THREAD_PTHREAD_RECURSIVE_MUTEX_NP @SDL_THREAD_PTHREAD_RECURSIVE_MUTEX_NP@
#cmakedefine SDL_THREAD_WINDOWS @SDL_THREAD_WINDOWS@

/* Enable various timer systems */
#cmakedefine SDL_TIMER_HAIKU @SDL_TIMER_HAIKU@
#cmakedefine SDL_TIMER_DUMMY @SDL_TIMER_DUMMY@
#cmakedefine SDL_TIMER_UNIX @SDL_TIMER_UNIX@
#cmakedefine SDL_TIMER_WINDOWS @SDL_TIMER_WINDOWS@
#cmakedefine SDL_TIMER_WINCE @SDL_TIMER_WINCE@

/* Enable various video drivers */
#cmakedefine SDL_VIDEO_DRIVER_ANDROID @SDL_VIDEO_DRIVER_ANDROID@
#cmakedefine SDL_VIDEO_DRIVER_HAIKU @SDL_VIDEO_DRIVER_HAIKU@
#cmakedefine SDL_VIDEO_DRIVER_COCOA @SDL_VIDEO_DRIVER_COCOA@
#cmakedefine SDL_VIDEO_DRIVER_DIRECTFB @SDL_VIDEO_DRIVER_DIRECTFB@
#cmakedefine SDL_VIDEO_DRIVER_DIRECTFB_DYNAMIC @SDL_VIDEO_DRIVER_DIRECTFB_DYNAMIC@
#cmakedefine SDL_VIDEO_DRIVER_DUMMY @SDL_VIDEO_DRIVER_DUMMY@
#cmakedefine SDL_VIDEO_DRIVER_WINDOWS @SDL_VIDEO_DRIVER_WINDOWS@
#cmakedefine SDL_VIDEO_DRIVER_WAYLAND @SDL_VIDEO_DRIVER_WAYLAND@
#cmakedefine SDL_VIDEO_DRIVER_RPI @SDL_VIDEO_DRIVER_RPI@
#cmakedefine SDL_VIDEO_DRIVER_VIVANTE @SDL_VIDEO_DRIVER_VIVANTE@
#cmakedefine SDL_VIDEO_DRIVER_VIVANTE_VDK @SDL_VIDEO_DRIVER_VIVANTE_VDK@

#cmakedefine SDL_VIDEO_DRIVER_KMSDRM @SDL_VIDEO_DRIVER_KMSDRM@
#cmakedefine SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC @SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC@
#cmakedefine SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC_GBM @SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC_GBM@

#cmakedefine SDL_VIDEO_DRIVER_WAYLAND_QT_TOUCH @SDL_VIDEO_DRIVER_WAYLAND_QT_TOUCH@
#cmakedefine SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC @SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC@
#cmakedefine SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL @SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_EGL@
#cmakedefine SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR @SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_CURSOR@
#cmakedefine SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON @SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC_XKBCOMMON@

#cmakedefine SDL_VIDEO_DRIVER_MIR @SDL_VIDEO_DRIVER_MIR@
#cmakedefine SDL_VIDEO_DRIVER_MIR_DYNAMIC @SDL_VIDEO_DRIVER_MIR_DYNAMIC@
#cmakedefine SDL_VIDEO_DRIVER_MIR_DYNAMIC_XKBCOMMON @SDL_VIDEO_DRIVER_MIR_DYNAMIC_XKBCOMMON@
#cmakedefine SDL_VIDEO_DRIVER_EMSCRIPTEN @SDL_VIDEO_DRIVER_EMSCRIPTEN@
#cmakedefine SDL_VIDEO_DRIVER_X11 @SDL_VIDEO_DRIVER_X11@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC @SDL_VIDEO_DRIVER_X11_DYNAMIC@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT @SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR @SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XINERAMA @SDL_VIDEO_DRIVER_X11_DYNAMIC_XINERAMA@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT2 @SDL_VIDEO_DRIVER_X11_DYNAMIC_XINPUT2@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR @SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS @SDL_VIDEO_DRIVER_X11_DYNAMIC_XSS@
#cmakedefine SDL_VIDEO_DRIVER_X11_DYNAMIC_XVIDMODE @SDL_VIDEO_DRIVER_X11_DYNAMIC_XVIDMODE@
#cmakedefine SDL_VIDEO_DRIVER_X11_XCURSOR @SDL_VIDEO_DRIVER_X11_XCURSOR@
#cmakedefine SDL_VIDEO_DRIVER_X11_XDBE @SDL_VIDEO_DRIVER_X11_XDBE@
#cmakedefine SDL_VIDEO_DRIVER_X11_XINERAMA @SDL_VIDEO_DRIVER_X11_XINERAMA@
#cmakedefine SDL_VIDEO_DRIVER_X11_XINPUT2 @SDL_VIDEO_DRIVER_X11_XINPUT2@
#cmakedefine SDL_VIDEO_DRIVER_X11_XINPUT2_SUPPORTS_MULTITOUCH @SDL_VIDEO_DRIVER_X11_XINPUT2_SUPPORTS_MULTITOUCH@
#cmakedefine SDL_VIDEO_DRIVER_X11_XRANDR @SDL_VIDEO_DRIVER_X11_XRANDR@
#cmakedefine SDL_VIDEO_DRIVER_X11_XSCRNSAVER @SDL_VIDEO_DRIVER_X11_XSCRNSAVER@
#cmakedefine SDL_VIDEO_DRIVER_X11_XSHAPE @SDL_VIDEO_DRIVER_X11_XSHAPE@
#cmakedefine SDL_VIDEO_DRIVER_X11_XVIDMODE @SDL_VIDEO_DRIVER_X11_XVIDMODE@
#cmakedefine SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS @SDL_VIDEO_DRIVER_X11_SUPPORTS_GENERIC_EVENTS@
#cmakedefine SDL_VIDEO_DRIVER_X11_CONST_PARAM_XEXTADDDISPLAY @SDL_VIDEO_DRIVER_X11_CONST_PARAM_XEXTADDDISPLAY@
#cmakedefine SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM @SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM@

#cmakedefine SDL_VIDEO_RENDER_D3D @SDL_VIDEO_RENDER_D3D@
#cmakedefine SDL_VIDEO_RENDER_D3D11 @SDL_VIDEO_RENDER_D3D11@
#cmakedefine SDL_VIDEO_RENDER_OGL @SDL_VIDEO_RENDER_OGL@
#cmakedefine SDL_VIDEO_RENDER_OGL_ES @SDL_VIDEO_RENDER_OGL_ES@
#cmakedefine SDL_VIDEO_RENDER_OGL_ES2 @SDL_VIDEO_RENDER_OGL_ES2@
#cmakedefine SDL_VIDEO_RENDER_DIRECTFB @SDL_VIDEO_RENDER_DIRECTFB@

/* Enable OpenGL support */
#cmakedefine SDL_VIDEO_OPENGL @SDL_VIDEO_OPENGL@
#cmakedefine SDL_VIDEO_OPENGL_ES @SDL_VIDEO_OPENGL_ES@
#cmakedefine SDL_VIDEO_OPENGL_ES2 @SDL_VIDEO_OPENGL_ES2@
#cmakedefine SDL_VIDEO_OPENGL_BGL @SDL_VIDEO_OPENGL_BGL@
#cmakedefine SDL_VIDEO_OPENGL_CGL @SDL_VIDEO_OPENGL_CGL@
#cmakedefine SDL_VIDEO_OPENGL_GLX @SDL_VIDEO_OPENGL_GLX@
#cmakedefine SDL_VIDEO_OPENGL_WGL @SDL_VIDEO_OPENGL_WGL@
#cmakedefine SDL_VIDEO_OPENGL_EGL @SDL_VIDEO_OPENGL_EGL@
#cmakedefine SDL_VIDEO_OPENGL_OSMESA @SDL_VIDEO_OPENGL_OSMESA@
#cmakedefine SDL_VIDEO_OPENGL_OSMESA_DYNAMIC @SDL_VIDEO_OPENGL_OSMESA_DYNAMIC@

/* Enable Vulkan support */
#cmakedefine SDL_VIDEO_VULKAN @SDL_VIDEO_VULKAN@

/* Enable system power support */
#cmakedefine SDL_POWER_ANDROID @SDL_POWER_ANDROID@
#cmakedefine SDL_POWER_LINUX @SDL_POWER_LINUX@
#cmakedefine SDL_POWER_WINDOWS @SDL_POWER_WINDOWS@
#cmakedefine SDL_POWER_MACOSX @SDL_POWER_MACOSX@
#cmakedefine SDL_POWER_HAIKU @SDL_POWER_HAIKU@
#cmakedefine SDL_POWER_EMSCRIPTEN @SDL_POWER_EMSCRIPTEN@
#cmakedefine SDL_POWER_HARDWIRED @SDL_POWER_HARDWIRED@

/* Enable system filesystem support */
#cmakedefine SDL_FILESYSTEM_ANDROID @SDL_FILESYSTEM_ANDROID@
#cmakedefine SDL_FILESYSTEM_HAIKU @SDL_FILESYSTEM_HAIKU@
#cmakedefine SDL_FILESYSTEM_COCOA @SDL_FILESYSTEM_COCOA@
#cmakedefine SDL_FILESYSTEM_DUMMY @SDL_FILESYSTEM_DUMMY@
#cmakedefine SDL_FILESYSTEM_UNIX @SDL_FILESYSTEM_UNIX@
#cmakedefine SDL_FILESYSTEM_WINDOWS @SDL_FILESYSTEM_WINDOWS@
#cmakedefine SDL_FILESYSTEM_EMSCRIPTEN @SDL_FILESYSTEM_EMSCRIPTEN@

/* Enable assembly routines */
#cmakedefine SDL_ASSEMBLY_ROUTINES @SDL_ASSEMBLY_ROUTINES@
#cmakedefine SDL_ALTIVEC_BLITTERS @SDL_ALTIVEC_BLITTERS@

/* Enable dynamic libsamplerate support */
#cmakedefine SDL_LIBSAMPLERATE_DYNAMIC @SDL_LIBSAMPLERATE_DYNAMIC@

/* Platform specific definitions */
#if !defined(__WIN32__)
#  if !defined(_STDINT_H_) && !defined(_STDINT_H) && !defined(HAVE_STDINT_H) && !defined(_HAVE_STDINT_H)
typedef unsigned int size_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;
#  endif /* if (stdint.h isn't available) */
#else /* __WIN32__ */
#  if !defined(_STDINT_H_) && !defined(HAVE_STDINT_H) && !defined(_HAVE_STDINT_H)
#    if defined(__GNUC__) || defined(__DMC__) || defined(__WATCOMC__)
#define HAVE_STDINT_H	1
#    elif defined(_MSC_VER)
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#      ifndef _UINTPTR_T_DEFINED
#        ifdef  _WIN64
typedef unsigned __int64 uintptr_t;
#          else
typedef unsigned int uintptr_t;
#        endif
#define _UINTPTR_T_DEFINED
#      endif
/* Older Visual C++ headers don't have the Win64-compatible typedefs... */
#      if ((_MSC_VER <= 1200) && (!defined(DWORD_PTR)))
#define DWORD_PTR DWORD
#      endif
#      if ((_MSC_VER <= 1200) && (!defined(LONG_PTR)))
#define LONG_PTR LONG
#      endif
#    else /* !__GNUC__ && !_MSC_VER */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#      ifndef _SIZE_T_DEFINED_
#define _SIZE_T_DEFINED_
typedef unsigned int size_t;
#      endif
typedef unsigned int uintptr_t;
#    endif /* __GNUC__ || _MSC_VER */
#  endif /* !_STDINT_H_ && !HAVE_STDINT_H */
#endif /* __WIN32__ */

#endif /* SDL_config_h_ */
