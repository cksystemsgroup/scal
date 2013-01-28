AC_INIT([Scal], 1.0)

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADER([config.h])

: ${CFLAGS=""}
: ${CXXFLAGS=""}

AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AC_LANG_SOURCE([C++])

dnl compiler debugging flags
AC_ARG_ENABLE([debug],
    AS_HELP_STRING([--enable-debug], [Include debugging symbols]))

CC_OPTIMIZATION="-O3"
AS_IF([test "x$enable_debug" = "xyes"], [
    CC_OPTIMIZATION="-ggdb"
])
AC_SUBST([CC_OPTIMIZATION])

dnl double width CAS
AC_ARG_ENABLE([cas128],
    AS_HELP_STRING([--disable-cas128], [Disable usage of 128bit CAS (and
                    use 64bit CAS instead)]))
AS_IF([test "x$enable_cas128" != "xno"], [
  AC_DEFINE([USE_CAS128], [1], [Use 128 bit CAS operations])
])

dnl uintxx_t types
AC_CHECK_HEADERS([stdint.h inttypes.h sys/types.h],
    [scal_found_int_headers=yes; break;])
AS_IF([test "x$scal_found_int_headers" != "xyes"],
    [AC_MSG_ERROR([Unable to find the standard integers headers])])

dnl pthreads
AC_CHECK_HEADERS([pthread.h],
    [scal_found_pthread=yes; break;])
AS_IF([test "x$scal_found_pthread" != "xyes"],
    [AC_MSG_ERROR([Unable to find pthread headers])])

AC_SEARCH_LIBS([pthread_create], [pthread dld], [], [
  AC_MSG_ERROR([unable to find the pthread_create() function])
])

dnl pkg-config (actually its pkg.m4) is needed for PKG_CHECK_MODULES
AC_CHECK_PROG([scal_have_pkg_config], [pkg-config], [yes])
AS_IF([test "x$scal_have_pkg_config" != "xyes"],
    [AC_MSG_ERROR([Unable to find pkg-config. Need the package since it ships with pkg.m4])])

dnl check for g++ template alias
dnl should be  supported from gcc >=4.7
OLD_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++0x"
AC_MSG_CHECKING([whether g++ supports template aliasing])
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
        [[template<typename T> class Test {};
          template<typename T> using Test2 = Test<T>;]],
        [[]]
    )],
    [
        AC_MSG_RESULT([yes])
        compiler_supports_template_alias="yes"
    ],[
        AC_MSG_RESULT([no])
    ])
CXXFLAGS="$OLD_CXXFLAGS"
AS_IF([test "x$compiler_supports_template_alias" = "xyes"], 
  AC_DEFINE([SUPPORTS_TEMPLATE_ALIAS], [1], [Compiler supports template aliasing])
])

dnl gflags
PKG_CHECK_MODULES([GFLAGS], [libgflags])
AC_SUBST([GFLAGS_CFLAGS])
AC_SUBST([GFLAGS_LIBS])

dnl sched_setscheduler
rtlibs="rt posix4"
AC_SEARCH_LIBS([sched_setscheduler], [$rtlibs], [], [
  AC_MSG_ERROR([unable to find sched_setscheduler() function of realtime
                extensions])
])

dnl math stuff
AC_SEARCH_LIBS([fabs], [m], [], [
  AC_MSG_ERROR([unable to find fabs() function of the math library])
])

dnl tcmalloc in google-perftools
tclibs="tcmalloc_minimal tcmalloc"
AC_SEARCH_LIBS([tc_cfree], [$tclibs], [], [
  AC_MSG_ERROR([unable to find tc_cfree() function of tcmalloc])
])

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
