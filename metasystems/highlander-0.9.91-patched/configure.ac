AC_PREREQ(2.57)
AC_INIT(highlander, 0.9.91, boa@metasystems.no)
AC_CONFIG_AUX_DIR([extras])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/internals.h])
# AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_RANLIB
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([strerror_r inet_ntop localtime_r socket strerror strtol sendfile])

# With or without threads
#AC_ARG_ENABLE(tiny, 
#[ --enable-tiny, Enable Tiny build],
#[case "${enableval}" in
	#yes) tiny=true ;;
	#no)  tiny=false ;;
	#*) AC_MSG_ERROR(bad value ${enableval} for --enable-tiny) ;;
	#esac],[tiny=false])
#AM_CONDITIONAL(TINY, test x$tiny = xtrue)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

