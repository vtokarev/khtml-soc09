/* config.h. Generated by cmake from config.h.cmake */

/* NOTE: only add something here if it is really needed by all of kdelibs.
   Otherwise please prefer adding to the relevant config-foo.h.cmake file,
   to minimize recompilations and increase modularity. */

/* if setgroups() takes short *as second arg */
#cmakedefine HAVE_SHORTSETGROUPS 1 

/****************************/

#define kde_socklen_t socklen_t

#define KDELIBSUFF "${KDELIBSUFF}"

/****************************/

#cmakedefine   HAVE_BZIP2_SUPPORT 1
/* Define if the libbz2 functions need the BZ2_ prefix */
#cmakedefine   NEED_BZ2_PREFIX 1

/* Define if you have libz */
#cmakedefine   HAVE_LIBZ 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine   HAVE_DLFCN_H 1

/* Define to 1 if you have stdio.h */
#cmakedefine   HAVE_STDIO_H 1

/* Define to 1 if you have stdlib.h */
#cmakedefine   HAVE_STDLIB_H 1

/* Define to 1 if you have string.h */
#cmakedefine   HAVE_STRING_H 1

/* Define to 1 if you have strings.h */
#cmakedefine   HAVE_STRINGS_H 1

/* Define to 1 if you have malloc.h */
#cmakedefine   HAVE_MALLOC_H 1

/* define if message translations are enabled */
#cmakedefine ENABLE_NLS 1

#cmakedefine HAVE_VOLMGT 1

#cmakedefine HAVE_ERRNO_H 1
#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_SYS_STAT_H 1
#cmakedefine HAVE_SYS_TYPES_H 1
#cmakedefine HAVE_SYS_PARAM_H 1
#cmakedefine HAVE_SYS_TIME_H 1
#cmakedefine HAVE_SYS_SELECT_H 1
#cmakedefine HAVE_SYSENT_H 1
#cmakedefine HAVE_SYS_MNTTAB_H 1
#cmakedefine HAVE_SYS_MNTENT_H 1
#cmakedefine HAVE_SYS_MOUNT_H 1
#cmakedefine HAVE_CRTDBG_H 1

#cmakedefine HAVE_ALLOCA_H 1
#cmakedefine HAVE_CRT_EXTERNS_H 1
#cmakedefine HAVE_CARBON_CARBON_H 1
#cmakedefine HAVE_FSTAB_H 1
#cmakedefine HAVE_LIMITS_H 1
#cmakedefine HAVE_MNTENT_H 1
#cmakedefine HAVE_NETINET_IN_H 1
#cmakedefine HAVE_PATHS_H 1
#cmakedefine HAVE_SYS_MMAN_H 1
#cmakedefine HAVE_SYS_UCRED_H 1
#cmakedefine HAVE_UNISTD_H 1
#cmakedefine HAVE_ARPA_NAMESER8_COMPAT_H
#cmakedefine HAVE_LANGINFO_H 1

#cmakedefine HAVE_XTEST 1

/* Define to 1 if you have the Xcursor library */
#cmakedefine HAVE_XCURSOR 1

/* Define to 1 if you have the Xfixes library */
#cmakedefine HAVE_XFIXES 1

/* Define to 1 if you have the Xrender library */
#cmakedefine HAVE_XRENDER 1

#cmakedefine   HAVE_BACKTRACE 1
#cmakedefine   HAVE_FADVISE 1
#cmakedefine   HAVE_GETMNTINFO 1
#cmakedefine   HAVE_GETPAGESIZE 1
#cmakedefine   HAVE_INITGROUPS 1
#cmakedefine   HAVE_MADVISE 1
#cmakedefine   HAVE_MMAP 1
#cmakedefine   HAVE_MKSTEMPS 1
#cmakedefine   HAVE_MKSTEMP 1
#cmakedefine   HAVE_MKDTEMP 1
#cmakedefine   HAVE_RANDOM 1
#cmakedefine   HAVE_READDIR_R 1
#cmakedefine   HAVE_SENDFILE 1
#cmakedefine   HAVE_SETENV 1
#cmakedefine   HAVE_SETEUID 1
#cmakedefine   HAVE_SETMNTENT 1
#cmakedefine   HAVE_SETPRIORITY 1
#cmakedefine   HAVE_SRANDOM 1
#cmakedefine   HAVE_STRCMP 1
#cmakedefine   HAVE_STRLCPY 1
#cmakedefine   HAVE_STRLCAT 1
#cmakedefine   HAVE_STRCASESTR 1
#cmakedefine   HAVE_STRRCHR 1
#cmakedefine   HAVE_STRTOLL 1
#cmakedefine   HAVE_UNSETENV 1
#cmakedefine   HAVE_USLEEP 1
#cmakedefine   HAVE_VSNPRINTF 1
#cmakedefine   HAVE_NSGETENVIRON 1
#cmakedefine   HAVE_GETTIMEOFDAY 1
#cmakedefine   HAVE_TRUNC 1

#cmakedefine HAVE_S_ISSOCK 1

#cmakedefine TIME_WITH_SYS_TIME 1

/* When building universal binaries, you must determine endianness at compile-time */
#ifdef __APPLE__
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#else
#undef WORDS_BIGENDIAN
#endif

#else
#cmakedefine WORDS_BIGENDIAN
#endif

/********* structs ******************/

#cmakedefine HAVE_STRUCT_UCRED 1

/*********************/

#ifndef HAVE_S_ISSOCK
#define HAVE_S_ISSOCK
#define S_ISSOCK(mode) (1==0)
#endif

/*
 * On HP-UX, the declaration of vsnprintf() is needed every time !
 */

#if !defined(HAVE_VSNPRINTF) || defined(hpux)
#if __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#endif
#ifdef __cplusplus
extern "C"
#endif
int vsnprintf(char *str, size_t n, char const *fmt, va_list ap);
#ifdef __cplusplus
extern "C"
#endif
int snprintf(char *str, size_t n, char const *fmt, ...);
#endif

#if defined(HAVE_NSGETENVIRON) && defined(HAVE_CRT_EXTERNS_H)
# include <sys/time.h>
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#endif

#cmakedefine GETMNTINFO_USES_STATVFS 1

#cmakedefine SIZEOF_TIME_T ${SIZEOF_TIME_T}

/* Defined to 1 if you have a tm_gmtoff member in struct tm */
#cmakedefine HAVE_TM_GMTOFF 1

/* Defined to 1 if you have a tm_zone member in struct tm */
#cmakedefine HAVE_STRUCT_TM_TM_ZONE 1

/* Defined to 1 if you have a d_type member in struct dirent */
#cmakedefine HAVE_DIRENT_D_TYPE 1


#include "kdecore/kdefakes.h"

