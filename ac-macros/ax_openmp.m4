dnl @synopsis AX_OPENMP([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl @summary determine how to compile programs using OpenMP
dnl
dnl This macro tries to find out how to compile programs that use
dnl OpenMP a standard API and set of compiler directives for parallel
dnl programming (see http://www-unix.mcs/)
dnl
dnl On success, it sets the
dnl OPENMP_CFLAGS/OPENMP_CXXFLAGS/OPENMP_F77FLAGS output variable to
dnl the flag (e.g. -omp) used both to compile *and* link OpenMP
dnl programs in the current language.
dnl
dnl NOTE: You are assumed to not only compile your program with these
dnl flags, but also link it with them as well.
dnl
dnl If you want to compile everything with OpenMP, you should set:
dnl
dnl     CFLAGS="$CFLAGS $OPENMP_CFLAGS" 
dnl     #OR#  CXXFLAGS="$CXXFLAGS $OPENMP_CXXFLAGS" 
dnl     #OR#  FFLAGS="$FFLAGS $OPENMP_FFLAGS" 
dnl
dnl (depending on the selected language).
dnl
dnl The user can override the default choice by setting the
dnl corresponding environment variable (e.g. OPENMP_CFLAGS).
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if an OpenMP
dnl flag is found, and ACTION-IF-NOT-FOUND is a list of commands to run
dnl it if it is not found. If ACTION-IF-FOUND is not specified, the
dnl default action will define HAVE_OPENMP.
dnl
dnl @category InstalledPackages
dnl @author Steven G. Johnson <stevenj@alum.mit.edu>
dnl @version 2006-01-24
dnl @license GPLWithACException

AC_DEFUN([AX_OPENMP], [
AC_PREREQ(2.59) dnl for _AC_LANG_PREFIX

AC_CACHE_CHECK([for OpenMP flag of _AC_LANG compiler], ax_cv_[]_AC_LANG_ABBREV[]_openmp, [save[]_AC_LANG_PREFIX[]FLAGS=$[]_AC_LANG_PREFIX[]FLAGS
ax_cv_[]_AC_LANG_ABBREV[]_openmp=unknown
# Flags to try:  -fopenmp (gcc), -openmp (icc), -mp (SGI & PGI),
#                -xopenmp (Sun), -omp (Tru64), -qsmp=omp (AIX), none
ax_openmp_flags="-fopenmp -openmp -mp -xopenmp -omp -qsmp=omp none"
if test "x$OPENMP_[]_AC_LANG_PREFIX[]FLAGS" != x; then
  ax_openmp_flags="$OPENMP_[]_AC_LANG_PREFIX[]FLAGS $ax_openmp_flags"
fi
for ax_openmp_flag in $ax_openmp_flags; do
  case $ax_openmp_flag in
    none) []_AC_LANG_PREFIX[]FLAGS=$save[]_AC_LANG_PREFIX[] ;;
    *) []_AC_LANG_PREFIX[]FLAGS="$save[]_AC_LANG_PREFIX[]FLAGS $ax_openmp_flag" ;;
  esac
  AC_TRY_LINK_FUNC(omp_set_num_threads,
	[ax_cv_[]_AC_LANG_ABBREV[]_openmp=$ax_openmp_flag; break])
done
[]_AC_LANG_PREFIX[]FLAGS=$save[]_AC_LANG_PREFIX[]FLAGS
])
if test "x$ax_cv_[]_AC_LANG_ABBREV[]_openmp" = "xunknown"; then
  m4_default([$2],:)
else
  if test "x$ax_cv_[]_AC_LANG_ABBREV[]_openmp" != "xnone"; then
    OPENMP_[]_AC_LANG_PREFIX[]FLAGS=$ax_cv_[]_AC_LANG_ABBREV[]_openmp
  fi
  m4_default([$1], [AC_DEFINE(HAVE_OPENMP,1,[Define if OpenMP is enabled])])
fi
])dnl AX_OPENMP
