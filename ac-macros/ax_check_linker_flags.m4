# ===========================================================================
#      http://www.nongnu.org/autoconf-archive/ax_check_linker_flags.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_LINKER_FLAGS(FLAGS, [ACTION-SUCCESS], [ACTION-FAILURE])
#
# DESCRIPTION
#
#   Check whether the given linker FLAGS work with the current language's
#   linker, or whether they give an error.
#
#   ACTION-SUCCESS/ACTION-FAILURE are shell commands to execute on
#   success/failure.
#
#   NOTE: Based on AX_CHECK_COMPILER_FLAGS.
#
# LICENSE
#
#   Copyright (c) 2009 Mike Frysinger <vapier@gentoo.org>
#   Copyright (c) 2009 Steven G. Johnson <stevenj@alum.mit.edu>
#   Copyright (c) 2009 Matteo Frigo
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 5

AC_DEFUN([AX_CHECK_LINKER_FLAGS],
[AC_MSG_CHECKING([whether the linker accepts $1])
dnl Some hackery here since AC_CACHE_VAL can't handle a non-literal varname:
AS_LITERAL_IF([$1],
  [AC_CACHE_VAL(AS_TR_SH(ax_cv_linker_flags_[$1]), [
      ax_save_FLAGS=$LDFLAGS
      LDFLAGS="$1"
      AC_LINK_IFELSE([AC_LANG_PROGRAM()],
        AS_TR_SH(ax_cv_linker_flags_[$1])=yes,
        AS_TR_SH(ax_cv_linker_flags_[$1])=no)
      LDFLAGS=$ax_save_FLAGS])],
  [ax_save_FLAGS=$LDFLAGS
   LDFLAGS="$1"
   AC_LINK_IFELSE([AC_LANG_PROGRAM()],
     eval AS_TR_SH(ax_cv_linker_flags_[$1])=yes,
     eval AS_TR_SH(ax_cv_linker_flags_[$1])=no)
   LDFLAGS=$ax_save_FLAGS])
eval ax_check_linker_flags=$AS_TR_SH(ax_cv_linker_flags_[$1])
AC_MSG_RESULT($ax_check_linker_flags)
if test "x$ax_check_linker_flags" = xyes; then
	m4_default([$2], :)
else
	m4_default([$3], :)
fi
])dnl AX_CHECK_LINKER_FLAGS
