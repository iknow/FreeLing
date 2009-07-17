#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([FreeLing],[2.1-beta1])
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE(FreeLing,2.1-beta1)

AC_CANONICAL_HOST	
AC_CONFIG_SRCDIR([src/main/sample_analyzer/analyzer.cc])

CPPFLAGS=" $CPPFLAGS -I/usr/include/db4 -I/usr/local/include"
CXXFLAGS=" $CXXFLAGS -O3 -Wall"
LDFLAGS=" $LDFLAGS -L/usr/local/lib"


######## Auxiliary functions ##########

AC_DEFUN([AC_JNI_DIRS],[
        # ---  Finds out where JNI include and lib dirs are located

	JNI_INCLUDE_DIRS=""
        JVM_LIB_DIR=""

	AC_MSG_CHECKING(for \$JAVAC)
	if test "x$JAVAC" = x; then
 	   AC_MSG_RESULT([undefined. Using 'javac' as default compiler name.])
	   export JAVAC=javac
	else
 	   AC_MSG_RESULT([$JAVAC])
        fi

	AC_PATH_PROG(_ACJNI_JAVAC, $JAVAC, no)
	test "x$_ACJNI_JAVAC" = xno && AC_MSG_ERROR([Java compiler '$JAVAC' could not be found in path])

	_AC_JNI_TOP("$_ACJNI_JAVAC")

	if test -f "$_JINC/jni.h"; then
	        JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JINC"
	else
	        _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
	        if test -f "$_JTOPDIR/include/jni.h"; then
	                JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include"
	        else
	                AC_MSG_ERROR([cannot find java include files])
	        fi
	fi

	# get the likely subdirectories for system specific java includes
	case "$host_os" in
	bsdi*)          _JNI_INC_SUBDIRS="bsdos";;
	linux*)         _JNI_INC_SUBDIRS="linux genunix";;
	osf*)           _JNI_INC_SUBDIRS="alpha";;
	solaris*)       _JNI_INC_SUBDIRS="solaris";;
	mingw*)                 _JNI_INC_SUBDIRS="win32";;
	cygwin*)                _JNI_INC_SUBDIRS="win32";;
	*)              _JNI_INC_SUBDIRS="genunix";;
	esac

	# add any subdirectories that are present
	for JINCSUBDIR in $_JNI_INC_SUBDIRS
	do
	        if test -d "$_JTOPDIR/include/$JINCSUBDIR"; then
	                JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include/$JINCSUBDIR"
	        fi
	done

 	JVM_LIB_DIR=`find $_JTOPDIR/ -name 'libjvm*' | grep client | sed -e 's:/[[^/]]*$::'`

	for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS; do
	  CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
	done

	LDFLAGS="$LDFLAGS -L$JVM_LIB_DIR"

]) #-- End AC_JNI_DIRS


AC_DEFUN([_AC_JNI_TOP],[
        # -- Follows symbolic links on given JAVAC path, and locates
        # -- the actual location of JDK

	# find the include directory relative to the javac executable
	_cur="$1"
	while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
	        AC_MSG_CHECKING(symlink for $_cur)
	        _slink=`ls -ld "$_cur" | sed 's/.* -> //'`
	        case "$_slink" in
	        /*) _cur="$_slink";;
	        # 'X' avoids triggering unwanted echo options.
	        *) _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
	        esac
	        AC_MSG_RESULT($_cur)
	done
	_ACJNI_FOLLOWED="$_cur"

	_JTOPDIR=`echo "$_ACJNI_FOLLOWED" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
	case "$host_os" in
	        darwin*)        _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
	                        _JINC="$_JTOPDIR/Headers";;
	        *)              _JINC="$_JTOPDIR/include";;
	esac

]) #-- End _AC_JNI_TOP



######## Begin checkings ##########

# check if maltplugin has to be compiled
AC_ARG_ENABLE([maltplugin],
              [AS_HELP_STRING([--enable-maltplugin],[Compile with plugin to call MaltParser from FreeLing (MaltParser and Java JDK must be installed on your system). Default is: Not enabled.])],
	      [if test "$enableval" = yes; then
	         # define symbol for CPP
                 AC_DEFINE(ENABLE_MALT)
		 # check for Java paths
		 AC_JNI_DIRS
               fi
	       # pass info to automake
	       AM_CONDITIONAL([ENABLE_MALT],[test "$enableval" = yes])
              ],
	      [AM_CONDITIONAL([ENABLE_MALT],[test 0 = 1])])

AC_ARG_ENABLE([traces],
              [AS_HELP_STRING([--enable-traces],[Compile with tracing information. Slows down the system, but you can know what's going on. Default is: Not enabled.])],
	      [if test "$enableval" = yes; then
	         # define symbol for CPP
                 AC_DEFINE(VERBOSE)
               fi
	       # pass info to automake
	       AM_CONDITIONAL([VERBOSE],[test "$enableval" = yes])
              ],
	      [AM_CONDITIONAL([VERBOSE],[test 0 = 1])])


AM_CONDITIONAL(OS_SOLARIS, [test x"$host_os" = xsolaris])

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
                      echo "   Make sure pcre headers (package libpcre3-dev or the like)" &&
                      echo "   are installed and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([db_cxx.h],[],[echo "" && echo "   db_cxx.h not found." &&
                      echo "   Make sure BerkeleyDB headers for C++ (package libdb4.6++-dev" &&
                      echo "   or the like) are installed and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([boost/filesystem.hpp],[],[echo "" && echo "   boost/filesystem.hpp not found." &&
                      echo "   Make sure libboost headers (package libboost-dev or the like)" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set CPPFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_HEADERS([boost/graph/graphviz.hpp],[],[echo "" && echo "   boost/graph not found." &&
                      echo "   Make sure libboost headers (package libboost-dev or the like)" &&
                      echo "   are installed and can be found in a standard path." &&
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
                      echo "   Make sure libpcre (package libpcre3 or the like)" &&
                      echo "   is installed and can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([db_cxx],[main],[echo "OK">/dev/null],[echo "" &&
                      echo "   db library not found." &&
                      echo "   Make sure Berkeley DB for C++ (package libdb4.6++, libdb_cxx" &&
                      echo "   or the like) is installed and can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([boost_filesystem],[main],[echo "OK">/dev/null],[echo "" &&
                      echo "   boost_filesystem library not found." &&
                      echo "   Make sure libboost_filesystem is" &&
                      echo "   installed and can be found in a standard path." &&
                      echo "   You may need to set LDFLAGS to specify search path." &&
                      echo "" && exit])
AC_CHECK_LIB([boost_graph],[main],[echo "OK">/dev/null],[echo "" &&
                      echo "   boost_graph library not found." &&
                      echo "   Make sure libboost_graph is" &&
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
                      echo "" && exit],[-lpcre])

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
