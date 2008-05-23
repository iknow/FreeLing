#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([FreeLing],[2.1-alfa1])
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE(FreeLing,2.1-alfa1)

AC_CONFIG_SRCDIR([src/main/analyzer.cc])
#AC_CONFIG_HEADER([config.h])

CPPFLAGS=" $CPPFLAGS -I/usr/include/db4 -I/usr/local/include"
CXXFLAGS=" $CXXFLAGS -O3 -Wall"
LDFLAGS=" $LDFLAGS -L/usr/local/lib"

# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LANG([C++])

# Checks for header files.
AC_CHECK_HEADERS([cfg+.h],[],[echo "" && echo "   cfg+.h not found." &&
                      echo "   Make sure libcfg+ headers are installed" &&
                      echo "   and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([pcre.h],[],[echo "" && echo "   pcre.h not found." &&
                      echo "   Make sure pcre headers for C++ are" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([db_cxx.h],[],[echo "" && echo "   db_cxx.h not found." &&
                      echo "   Make sure BerkeleyDB headers for C++ are" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([omlet.h],[],[echo "" && echo "   omlet.h not found." &&
                      echo "   Make sure libomlet headers are installed" &&
                      echo "   and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([fries.h],[],[echo "" && echo "   fries.h not found." &&
                      echo "   Make sure libfries headers are installed" &&
                      echo "   and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])

# Checks for libraries.
AC_CHECK_LIB([cfg+],[main],[echo "OK">/dev/null],[echo "" && 
                      echo "   cfg+ library not found." &&
                      echo "   Make sure libcfg+ is installed and" &&
                      echo "   can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([pcre],[main],[echo "OK">/dev/null],[echo "" && 
                      echo "   pcre library not found." &&
                      echo "   Make sure libpcre is installed and" &&
                      echo "   can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([db_cxx],[main],[echo "OK">/dev/null],[echo "" &&
                      echo "   db library not found." &&
                      echo "   Make sure libdb_cxx (Berkeley DB for C++) is" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([omlet],[main],[echo "OK">/dev/null],[echo "" &&
                      echo "   Omlet library not found." &&
                      echo "   Make sure libomlet is" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([fries],[main],[echo "OK">/dev/null],[echo "" &&
                      echo "   Fries library not found." &&
                      echo "   Make sure libfries is" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_HEADER_STDC

AC_CONFIG_FILES([Makefile
                 data/Makefile
                 doc/Makefile
                 src/Makefile
                 src/include/Makefile
                 src/libmorfo/Makefile
                 src/main/Makefile
                 src/utilities/Makefile])
AC_OUTPUT
