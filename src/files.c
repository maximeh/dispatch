/*
 Author:     Maxime Hadjinlian
             maxime.hadjinlian@gmail.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. The name of the author may not be used to endorse or promote products
    derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "files.h"
#include <errno.h>         // for errno, EINTR
#include <fcntl.h>         // for open, posix_fadvise, posix_fallocate, O_CREAT
#include <libgen.h>        // for dirname
#include <limits.h>        // for PATH_MAX
#include <stdio.h>         // for fprintf, stderr
#include <stdlib.h>        // for size_t, NULL
#include <string.h>        // for strerror, strlen, strncpy, strrchr
#include <sys/sendfile.h>  // for sendfile
#include <sys/stat.h>      // for mkdir, S_IRWXU
#include <unistd.h>        // for close, off_t
#include "tools.h"         // for DPRINTF

int
mkdir_p(const char *path)
{
	char opath[PATH_MAX];
	char *dname;
	const size_t len = strlen(path);

	strncpy(opath, path, len);
	dname = dirname(opath);
	char *p = dname;

	for (; *p; ++p) {
		if (*p != '/')
			continue;
		*p = '\0';
		DPRINTF(2, "create: %s\n", dname);
		mkdir(dname, S_IRWXU);
		*p = '/';
	}
	DPRINTF(2, "create: %s\n", dname);
	mkdir(dname, S_IRWXU);
	return 0;
}

const char *
get_filename_ext(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	return (!dot || dot == filename) ? NULL: ++dot;
}

int
copy(const char *src, const char *dest, const off_t filesize)
{
	int err = 0;
	int in, out;
	off_t ofs = 0;

	in = open(src, O_RDONLY);
	if (in == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}

	out = open(dest, O_WRONLY | O_CREAT);
	if (out == -1) {
		close(in);
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}

	if ((err = posix_fadvise(in, 0, filesize, POSIX_FADV_WILLNEED))) {
		fprintf(stderr, "ERROR: %s\n", strerror(err));
		goto close_fds;
	}

	if ((err = posix_fadvise(in, 0, filesize, POSIX_FADV_SEQUENTIAL))) {
		fprintf(stderr, "ERROR: %s\n", strerror(err));
		goto close_fds;
	}

	if ((err = posix_fallocate(out, 0, filesize))) {
		fprintf(stderr, "ERROR: %s\n", strerror(err));
		goto close_fds;
	}

	while(ofs < filesize) {
		if(sendfile(out, in, &ofs, (size_t)(filesize - ofs)) == -1) {
			if (errno == EINTR) {
				err = EINTR;
				goto close_fds;
			}
			continue;
		}
	}

close_fds:
	close(in);
	close(out);
	return err;
}
