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

#ifndef SDL_config_winrt_h_
#define SDL_config_winrt_h_
#define SDL_config_h_

#include "SDL_platform.h"

/* Make sure the Windows SDK's NTDDI_VERSION macro gets defined.  This is used
   by SDL to determine which version of the Windows SDK is being used.
*/
#include <sdkddkver.h>

/* Define possibly-undefined NTDDI values (used when compiling SDL against
   older versions of the Windows SDK.
*/
#ifndef NTDDI_WINBLUE
#define NTDDI_WINBLUE 0x06030000
#endif
#ifndef NTDDI_WIN10
#define NTDDI_WIN10 0x0A000000
#endif

/* This is a set of defines to configure the SDL features */

#if !defined(_STDINT_H_) && (!defined(HAVE_STDINT_H) || !_HAVE_STDINT_H)
#if defined(__GNUC__) || defined(__DMC__) || defined(__WATCOMC__)
#define HAVE_STDINT_H	1
#elif defined(_MSC_VER)
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 uintptr_t;
#else
typedef unsigned int uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif
/* Older Visual C++ headers don't have the Win64-compatible typedefs... */
#if ((_MSC_VER <= 1200) && (!defined(DWORD_PTR)))
#define DWORD_PTR DWORD
#endif
#if ((_MSC_VER <= 1200) && (!defined(LONG_PTR)))
#define LONG_PTR LONG
#endif
#else /* !__GNUC__ && !_MSC_VER */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#ifndef _SIZE_T_DEFINED_
#define _SIZE_T_DEFINED_
typedef unsigned int size_t;
#endif
typedef unsigned int uintptr_t;
#endif /* __GNUC__ || _MSC_VER */
#endif /* !_STDINT_H_ && !HAVE_STDINT_H */

#ifdef _WIN64
# define SIZEOF_VOIDP 8
#else
# define SIZEOF_VOIDP 4
#endif

/* Useful headers */
#define HAVE_DXGI_H 1
#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
#define HAVE_XINPUT_H 1
#endif
#define HAVE_LIBC 1
#define HAVE_STDIO_H 1
#define STDC_HEADERS 1
#define HAVE_STRING_H 1
#define HAVE_CTYPE_H 1
#define HAVE_MATH_H 1
#define HAVE_FLOAT_H 1
#define HAVE_SIGNAL_H 1

/* C library functions */
#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
#define HAVE_ALLOCA 1
#define HAVE_QSORT 1
#define HAVE_ABS 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_STRLEN 1
#define HAVE__STRREV 1
#define HAVE__STRUPR 1
//#define HAVE__STRLWR 1	// TODO, WinRT: consider using _strlwr_s instead
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
//#define HAVE_ITOA 1   // TODO, WinRT: consider using _itoa_s instead
//#define HAVE__LTOA 1	// TODO, WinRT: consider using _ltoa_s instead
//#define HAVE__ULTOA 1	// TODO, WinRT: consider using _ultoa_s instead
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
//#define HAVE_STRTOLL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE__STRICMP 1
#define HAVE__STRNICMP 1
#define HAVE_VSNPRINTF 1
//#define HAVE_SSCANF 1	// TODO, WinRT: consider using sscanf_s instead
#define HAVE_M_PI 1
#define HAVE_ATAN 1
#define HAVE_ATAN2 1
#define HAVE_CEIL 1
#define HAVE__COPYSIGN 1
#define HAVE_COS 1
#define HAVE_COSF 1
#define HAVE_FABS 1
#define HAVE_FLOOR 1
#define HAVE_LOG 1
#define HAVE_POW 1
//#define HAVE_SCALBN 1
#define HAVE__SCALB 1
#define HAVE_SIN 1
#define HAVE_SINF 1
#define HAVE_SQRT 1
#define HAVE_SQRTF 1
#define HAVE_TAN 1
#define HAVE_TANF 1
#define HAVE__FSEEKI64 1

/* Enable various audio drivers */
#define SDL_AUDIO_DRIVER_XAUDIO2	1
#define SDL_AUDIO_DRIVER_DISK	1
#define SDL_AUDIO_DRIVER_DUMMY	1

/* Enable various input drivers */
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#define SDL_JOYSTICK_DISABLED 1
#define SDL_HAPTIC_DISABLED	1
#else
#define SDL_JOYSTICK_XINPUT 1
#define SDL_HAPTIC_XINPUT   1
#endif

/* Enable various shared object loading systems */
#define SDL_LOADSO_WINDOWS	1

/* Enable various threading systems */
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
#define SDL_THREAD_WINDOWS  1
#else
/* WinRT on Windows 8.0 and Windows Phone 8.0 don't support CreateThread() */
#define SDL_THREAD_STDCPP   1
#endif

/* Enable various timer systems */
#define SDL_TIMER_WINDOWS	1

/* Enable various video drivers */
#define SDL_VIDEO_DRIVER_WINRT	1
#define SDL_VIDEO_DRIVER_DUMMY  1

/* Enable OpenGL ES 2.0 (via a modified ANGLE library) */
#define SDL_VIDEO_OPENGL_ES2 1
#define SDL_VIDEO_OPENGL_EGL 1

/* Enable appropriate renderer(s) */
#define SDL_VIDEO_RENDER_D3D11  1

#if SDL_VIDEO_OPENGL_ES2
#define SDL_VIDEO_RENDER_OGL_ES2 1
#endif

/* Enable system power support */
#define SDL_POWER_WINRT 1

/* Enable assembly routines (Win64 doesn't have inline asm) */
#ifndef _WIN64
#define SDL_ASSEMBLY_ROUTINES	1
#endif

#endif /* SDL_config_winrt_h_ */
