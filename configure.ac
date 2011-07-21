#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([libKdmGcc], [0.0.15], [bugs@kdmanalytics.com])
AM_INIT_AUTOMAKE
LT_PREREQ([2.2])
LT_INIT([shared])
AC_CONFIG_SRCDIR([source/gcckdm/GccKdmPlugin.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])




# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL


# Checks for libraries.
AX_BOOST_BASE([1.40])
AX_BOOST_FILESYSTEM

# Checks for header files.

if test "x$GCC_SRC_ROOT" = "x"; then
  plugin_includes="-I`$CXX -print-file-name=plugin`/include -I`$CXX -print-file-name=plugin`/include/c-family"
  AC_SUBST(GCC_PLUGIN_INCLUDES, $plugin_includes)
else
  AC_SUBST(GCC_PLUGIN_INCLUDES, $plugin_includes)
  plugin_includes="-I$GCC_SRC_ROOT/include -I$GCC_SRC_ROOT/include/c-family -I$GCC_SRC_ROOT/gcc  -I$GCC_SRC_ROOT/libcpp"
fi


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 source/Makefile])
AC_OUTPUT
