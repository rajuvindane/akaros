/* Copyright (C) 1991, 1995, 1996, 1997, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ros/syscall.h>

/* Perform file control operations on FD.  */
int
__fcntl(int fd, int cmd, ...)
{
	int ret, arg, advise;
	__off64_t offset, len;
	va_list vl;
	va_start(vl, cmd);
	switch (cmd) {
		case F_GETFD:
		case F_SYNC:
			ret = ros_syscall(SYS_fcntl, fd, cmd, 0, 0, 0, 0);
			break;
		case F_DUPFD:
		case F_SETFD:
		case F_GETFL:
		case F_SETFL:
			arg = va_arg(vl, int);
			ret = ros_syscall(SYS_fcntl, fd, cmd, arg, 0, 0, 0);
			break;
		case F_ADVISE:
			offset = va_arg(vl, __off64_t);
			len = va_arg(vl, __off64_t);
			advise = va_arg(vl, int);
			ret = ros_syscall(SYS_fcntl, fd, cmd, offset, len, advise, 0);
			break;
		default:
			errno = ENOSYS;
			ret = -1;
	}
	va_end(vl);
	return ret;
}

libc_hidden_def (__fcntl)
weak_alias (__fcntl, fcntl)
