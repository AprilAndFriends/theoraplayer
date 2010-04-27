AC_PREREQ(2.61)
AC_INIT([libtheoraplayer],[1.0],[kresimir.spes@gmail.com])
AC_CONFIG_SRCDIR([src/TheoraWorkerThread.cpp])
AM_INIT_AUTOMAKE([foreign])

AC_REVISION($Revision: 1.0 $)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG(C++)


# Check for Darwin.
AC_CANONICAL_HOST
AM_CONDITIONAL([DARWIN], [echo "$host_os" | grep '^darwin'])

# Checks for header files.
AC_HEADER_STDC
# Checks for header files.
AC_CHECK_HEADERS([memory.h string.h sys/time.h])
# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memset strstr])
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T


# Check for ogg
AC_CHECK_HEADERS([ogg/ogg.h], ,[AC_MSG_ERROR("*** Ogg headers not found.")])
AC_CHECK_LIB(ogg, ogg_sync_pageseek, ,[AC_MSG_ERROR("*** Ogg library not found or wrong version.")])

# Check for vorbis
AC_CHECK_HEADERS([vorbis/codec.h], ,[AC_MSG_ERROR("*** Vorbis headers not found.")])
AC_CHECK_LIB(vorbis, vorbis_comment_init, ,[AC_MSG_ERROR("*** Vorbis library not found or wrong version.")])

# Check for theora
AC_CHECK_HEADERS([theora/theoradec.h], ,[AC_MSG_ERROR("*** Theora headers not found.")])
AC_CHECK_LIB(theoradec, th_decode_free, ,[AC_MSG_ERROR("*** Theora library not found or wrong version.")])

# Output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	include/Makefile
])

AC_OUTPUT
