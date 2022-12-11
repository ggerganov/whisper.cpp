AC_INIT([whisper],[1.0a],[ggerganov@gmail.com])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([ac-macros])

# Don't clutter this dir, store in build-aux
AC_CONFIG_AUX_DIR([build-aux])

# Detect the canonical host and target build environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([-Wall -Werror])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_LANG(C++)

# Check for SDL (minimum: 2.0.0)
SDL2_VERSION=2.0.0
AM_PATH_SDL2($SDL2_VERSION, :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION or later was not found!]))
AC_SUBST(SDL2_CFLAGS)
AC_SUBST(SDL2_LIBS)

# Check for standard C++ library
AC_CHECK_LIB(stdc++, main)

# Stuff provided by autoscan
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([sqrt])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_C_RESTRICT
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_PROG_CPP
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Best optimization flags for our compiler
AX_CXXFLAGS_GCC_OPTION(-std=c++20)
AX_CXXFLAGS_GCC_OPTION(-Wall)
AX_CXXFLAGS_GCC_OPTION(-Wextra)
# AX_CXXFLAGS_GCC_OPTION(-pedantic)
#
# Give me speed!
AX_CFLAGS_GCC_OPTION(-fomit-frame-pointer)
AX_CFLAGS_GCC_OPTION(-ffast-math)
AX_CFLAGS_GCC_OPTION(-funsafe-math-optimizations)
AX_CFLAGS_GCC_OPTION(-mtune=native)
AX_CFLAGS_GCC_OPTION(-O3)

AX_CXXFLAGS_GCC_OPTION(-fomit-frame-pointer)
AX_CXXFLAGS_GCC_OPTION(-ffast-math)
AX_CXXFLAGS_GCC_OPTION(-funsafe-math-optimizations)
AX_CXXFLAGS_GCC_OPTION(-mtune=native)
AX_CXXFLAGS_GCC_OPTION(-O3)


AC_MSG_CHECKING(for SSE)
AC_RUN_IFELSE([AC_LANG_SOURCE([#include <stdlib.h>
#include <signal.h>
void catchme(int) { exit(1); }
int main(void) {
signal(SIGILL, catchme);
__asm__ volatile ("xorps %%xmm0, %%xmm0":::"memory"); return 0;
}
])], [HAVESSE=yes], [HAVESSE=no])
AC_MSG_RESULT($HAVESSE)
if test x"${HAVESSE}" = xyes ; then
        AX_CFLAGS_GCC_OPTION(-msse)
        AX_CXXFLAGS_GCC_OPTION(-msse)
        AC_DEFINE([USE_SSE], 1, [Defined when SSE instructions are available.])
fi

# Drop any asserts
CFLAGS="$CFLAGS -DNDEBUG"
CXXFLAGS="$CXXFLAGS -DNDEBUG"

# Finally create the generated Makefile from Makefile.am
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
