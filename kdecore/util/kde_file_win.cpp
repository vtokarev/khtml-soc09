/*
   This file is part of the KDE libraries
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2009 Christian Ehrlicher <ch.ehrlicher@gmx.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// needed for _wstat64
#define __MSVCRT_VERSION__ 0x601

#include "kde_file.h"

#include <QtCore/QFile>
#include <errno.h>

#include <sys/utime.h>
#include <sys/stat.h>
#include <wchar.h>
#define CONV(x) ((wchar_t*)x.utf16())

/** @internal, from kdewin32 lib */
static int kdewin_fix_mode_string(char *fixed_mode, const char *mode)
{
	if (strlen(mode)<1 || strlen(mode)>3) {
        errno = EINVAL;
		return 1;
    }

	strncpy(fixed_mode, mode, 3);
	if (fixed_mode[0]=='b' || fixed_mode[1]=='b' || fixed_mode[0]=='t' || fixed_mode[1]=='t')
		return 0;
	/* no 't' or 'b': append 'b' */
	if (fixed_mode[1]=='+') {
		fixed_mode[1]='b';
		fixed_mode[2]='+';
		fixed_mode[3]=0;
	}
	else {
		fixed_mode[1]='b';
		fixed_mode[2]=0;
	}
	return 0;
}

/** @internal */
static int kdewin_fix_flags(int flags)
{
	if ((flags & O_TEXT) == 0 && (flags & O_BINARY) == 0)
		return flags | O_BINARY;
	return flags;
}

namespace KDE
{
  int lstat(const QString &path, KDE_struct_stat *buf)
  {
    return KDE::stat( path, buf );
  }

  int mkdir(const QString &pathname, mode_t)
  {
    return _wmkdir( CONV(pathname) );
  }

  int open(const QString &pathname, int flags, mode_t mode)
  {
    return _wopen( CONV(pathname), kdewin_fix_flags(flags), mode );
  }

  int rename(const QString &in, const QString &out)
  {
	if ((0 == _waccess( CONV(out), 0)) &&
        (0 != _wremove( CONV(out) )))
		return -1;
	return _wrename( CONV(in), CONV(out) );
  }

  int stat(const QString &path, KDE_struct_stat *buf)
  {
    int result;
#ifdef Q_CC_MSVC
    struct _stat64 s64;
#else
    struct __stat64 s64;
#endif
    const int len = path.length();
    if ( (len==2 || len==3) && path[1]==':' && path[0].isLetter() ) {
    	/* 1) */
        QString newPath(path);
    	if (len==2)
    		newPath += QLatin1Char('\\');
    	result = _wstat64( CONV(newPath), &s64 );
    } else
    if ( len > 1 && (path.endsWith(QLatin1Char('\\')) || path.endsWith(QLatin1Char('/'))) ) {
    	/* 2) */
        const QString newPath = path.left( len - 1 );
    	result = _wstat64( CONV(newPath), &s64 );
    } else {
        //TODO: is stat("/") ok?
        result = _wstat64( CONV(path), &s64 );
    }
    if( result != 0 )
        return result;
    // KDE5: fixme!
    buf->st_dev   = s64.st_dev;
    buf->st_ino   = s64.st_ino;
    buf->st_mode  = s64.st_mode;
    buf->st_nlink = s64.st_nlink;
    buf->st_uid   = -2; // be in sync with Qt4
    buf->st_gid   = -2; // be in sync with Qt4
    buf->st_rdev  = s64.st_rdev;
    buf->st_size  = s64.st_size;
    buf->st_atime = s64.st_atime;
    buf->st_mtime = s64.st_mtime;
    buf->st_ctime = s64.st_ctime;
    return result;
  }
};