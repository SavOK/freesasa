#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([FreeSASA], [1.1])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/freesasa.c])
AC_CONFIG_HEADERS([config.h])
m4_include([m4/ax_compare_version.m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([inttypes.h libintl.h malloc.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_C_RESTRICT
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# use C99
AC_PROG_CC_C99
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset mkdir sqrt strchr strdup strerror getopt_long getline])

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile doc/Doxyfile
                 tests/Makefile bindings/Makefile share/Makefile
                 bindings/python/setup.py])
AC_CONFIG_FILES([tests/test-cli], [chmod +x tests/test-cli])
AC_CONFIG_FILES([bindings/check-python], [chmod +x bindings/check-python])

# Disable thread support
AC_ARG_ENABLE([threads],
  AS_HELP_STRING([--disable-threads],
    [Build without support for multiple threads]))

if test "x$enable_threads" != "xno" ; then
  AC_CHECK_LIB([pthread], [pthread_create])
  AC_CHECK_HEADERS([pthread.h])
  AC_DEFINE([USE_THREADS],[1],[Define if threads should be used.])
  AM_CONDITIONAL([USE_THREADS],true)
else
  AC_DEFINE([USE_THREADS],[0],[])
  AM_CONDITIONAL([USE_THREADS],false)
fi

# Which Python
AC_ARG_WITH([python],
  AS_HELP_STRING([--with-python=<python-binary>], [Specify which python to use.]),
    [python_bin=$withval],[python_bin=python])

# Enable Python-bindings
AC_ARG_ENABLE([python-bindings],
  [AS_HELP_STRING([--enable-python-bindings],
    [Build Python bindings using Cython])])

if test "x$enable_python_bindings" = "xyes" ; then
  AC_CHECK_PROG(PYTHON, python, $python_bin)
  AC_CHECK_PROG(CYTHON, cython, cython)
  AM_CONDITIONAL([HAVE_CYTHON], [test -n "$CYTHON" && test -n "$PYTHON"])
  AM_COND_IF([HAVE_CYTHON],[],[echo "Cython not available, will not build Python bindings"])
else
  AM_CONDITIONAL([HAVE_CYTHON],false)
fi

# Enable parser generation with Flex/Bison
AC_ARG_ENABLE([parser-generator],
  [AS_HELP_STRING([--enable-parser-generator],
    [Regenerate parser/lexer using Flex and Bison (for developers)])])

if test "x$enable_parser_generator" = "xyes" ; then
  AC_PROG_LEX
  AC_PROG_YACC
  AM_CONDITIONAL([GENERATE_PARSER], [test -n "$LEX" && test -n "$YACC"])
  AM_COND_IF([GENERATE_PARSER],[],[echo "Lexer and/or parser generator missing, won't generate parser source (but parser can still be built from the provided sources)."])
else
  AM_CONDITIONAL([GENERATE_PARSER],false)
fi

# Enable Doxygen
AC_ARG_ENABLE([doxygen],
  [AS_HELP_STRING([--enable-doxygen],
    [Build Doxygen html-documentation])])

if test "x$enable_doxygen" = "xyes" ; then
  AC_CHECK_PROG(DOXYGEN, doxygen, doxygen)
  AM_CONDITIONAL([HAVE_DOXYGEN],[test -n "$DOXYGEN"])
  AM_COND_IF([HAVE_DOXYGEN],[build_doc=yes],[echo "Doxygen not available, can not build documentation."])

  if test -n "$DOXYGEN"; then
     DOXYGEN_V=`doxygen --version`
     DOXYGEN_REQ=1.8.8
     AX_COMPARE_VERSION([$DOXYGEN_V],[lt],[$DOXYGEN_REQ],
       [echo "Warning: Doxygen should preferrably be of version $DOXYGEN_REQ or higher (found $DOXYGEN_V)"])
  fi
else
  AM_CONDITIONAL([HAVE_DOXYGEN],false)
fi

AM_CONDITIONAL([BUILD_DOC],[test -n "$build_doc"])

# Enable testing
AC_ARG_ENABLE([check],
  [AS_HELP_STRING([--enable-check],
    [Enable the Check unit testing framework])])

if test x$enable_check = xyes ; then
  AC_CHECK_LIB([check],[suite_create],
    [have_check=yes],
    [echo "Check not availabe, option '--enable-check' ignored"])
  AC_CHECK_LIB(rt, setitimer) # necessary on some platforms
  AM_CONDITIONAL([USE_CHECK],[test x$have_check = xyes])
else
  AM_CONDITIONAL([USE_CHECK],false)
fi

# Enable gcov
AC_ARG_ENABLE([gcov],
  [AS_HELP_STRING([--enable-gcov],
    [Compile with Gcov to measure coverage])])
AM_CONDITIONAL([COND_GCOV],[test "x$enable_gcov" = "xyes"])

# Disable test for CLI
AC_ARG_ENABLE([CLI-tests],
  AS_HELP_STRING([--disable-CLI-tests],
    [Disable testing of command-line interface. These take time and
     disabling them can help speed up development when only unit
     tests are needed.]))
if test "x$enable_CLI_tests" != "xno" ; then
   AM_CONDITIONAL([RUN_CLI_TESTS],true)
else
   AM_CONDITIONAL([RUN_CLI_TESTS],false)
fi

AC_OUTPUT
