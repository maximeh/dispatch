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

/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#define _XOPEN_SOURCE 700

#include <errno.h>         // for errno, EINVAL
#include <ftw.h>           // for nftw, FTW (ptr only), FTW_D, FTW_DNR, etc
#include <getopt.h>        // for getopt, optind, optarg
#include <limits.h>        // for PATH_MAX
#include <stdio.h>         // for fprintf, stderr, fputs, printf, sprintf, etc
#include <stdlib.h>        // for EXIT_SUCCESS
#include <string.h>        // for memcpy, memset, strerror, strlen, strstr
#include <sys/stat.h>      // for stat
#include <taglib/tag_c.h>  // for taglib_file_free, taglib_file_is_valid, etc

#include "files.h"         // for copy, get_filename_ext, mkdir_p
#include "tools.h"         // for append_esc, DPRINTF, append, filename

/* POSIX.1 says each process has at least 20 file descriptors.
 * Three of those belong to the standard streams.
 * Here, we use a conservative estimate of 15 available;
 * assuming we use at most two for other uses in this program,
 * we should never run into any problems.
 * Most trees are shallower than that, so it is efficient.
 * Deeper trees are traversed fine, just a bit slower.
 * (Linux allows typically hundreds to thousands of open files,
 *  so you'll probably never see any issues even if you used
 *  a much higher value, say a couple of hundred, but
 *  15 is a safe, reasonable value.)
*/
#ifndef USE_FDS
#define USE_FDS 15
#endif

int _debug = 0;
static const char *ALLOWED_EXTENSIONS = "mp3m4aflacogg";
static const char *DEFAULT_FMT = "%a/%A/%t - %T";

static char *FMT = NULL;
static char *DST = NULL;

static void
usage(void)
{
	(void)fprintf(stdout,
			"dispatch [-h] [-d] [-f FORMAT] SOURCE DEST\n");
	fputs("\t-h\t\tThis usage statement\n"
			"\t-d\t\tDebug output (-dd for more, ...)\n"
			"\t-f\t\tPath format - Tag supported:\n"
			"\t\t\t\t- %a (artist)\n"
			"\t\t\t\t- %A (album)\n"
			"\t\t\t\t- %y (year)\n"
			"\t\t\t\t- %t (track)\n"
			"\t\t\t\t- %T (title)\n",
			stdout);
}

static int
build_path_from_tag(const char *filepath, struct filename *fn)
{
	TagLib_File *file;
	TagLib_Tag *tag;
	int ret = -1;
	char *p = FMT;

	file = taglib_file_new(filepath);
	if (!file) {
		fprintf(stderr, "'%s' type cannot be determined"
				" or file cannot be opened.\n", filepath);
		goto free_taglib;

	}

	if (!taglib_file_is_valid(file)) {
		fprintf(stderr, "The file %s is not valid\n", filepath);
		goto free_taglib;
	}

	tag = taglib_file_tag(file);
	if (!tag) {
		fprintf(stderr, "The file %s is not valid.\n", filepath);
		goto free_taglib;
	}

	while (*p) {
		if (*p != '%') {
			ret = append(fn, "%c", *p++);
		} else {
			switch (*++p) {
			case 'a':
				ret = append_esc(fn, "%s",
						taglib_tag_artist(tag));
				break;
			case 'A':
				ret = append_esc(fn, "%s",
						taglib_tag_album(tag));
				break;
			case 't':
				ret = append_esc(fn, "%02d",
						taglib_tag_track(tag));
				break;
			case 'T':
				ret = append_esc(fn, "%s",
						taglib_tag_title(tag));
				break;
			case 'y':
				ret = append_esc(fn, "%d",
						taglib_tag_year(tag));
				break;
			default:
				break;
			}
			++p;
		}

		if (ret)
			goto free_taglib;
	}

free_taglib:
	taglib_tag_free_strings();
	taglib_file_free(file);
	return ret;
}

static int
dispatch_entry(const char *filepath, const struct stat *info,
		const int typeflag, struct FTW *pathinfo)
{
	const off_t filesize = info->st_size;
	const char *extension;
	struct filename fn;
	char *final_path;

	/* Don't bother to handle links */
	if (typeflag == FTW_SL)
		return 0;

	if (typeflag == FTW_SLN) {
		DPRINTF(2, "%s (dangling symlink)\n", filepath);
		return 0;
	}

	if (typeflag == FTW_D || typeflag == FTW_DP) {
		DPRINTF(2, "directory: %s/\n", filepath);
		return 0;
	}

	if (typeflag == FTW_DNR) {
		DPRINTF(2, "%s/ (unreadable)\n", filepath);
		return 0;
	}

	if (typeflag == FTW_F) {
		extension = get_filename_ext(filepath);
		if (!extension) {
			DPRINTF(1, "'%s' doesn't have an extension.\n",
					filepath);
			return 0;
		}

		/* Check that the extension is valid */
		if (!strstr(ALLOWED_EXTENSIONS, extension)) {
			DPRINTF(1, "'%s' doesn't have a valid extension.\n"
					"Valid extension are: "
					"mp3, m4a, flac and ogg\n",
					filepath);
			return 0;
		}

		/* Okey, so we are should be able to treat this file */
		fn.path = calloc(PATH_MAX, sizeof(char));
		if (!fn.path)
			return -ENOMEM;
		fn.used = 0;

		if (build_path_from_tag(filepath, &fn)) {
			free(fn.path);
			return 0;
		}

		final_path = calloc(PATH_MAX, sizeof(char));
		sprintf(final_path, "%s/%s.%s", DST, fn.path, extension);

		free(fn.path);
		fn.path = NULL;

		/* We have a path, yeah ! Now copy that sucker to its final
		   destination */
		printf("%s => %s\n", filepath, final_path);
		mkdir_p(final_path);
		if (copy(filepath, final_path, filesize))
			fprintf(stderr, "Could not copy %s to %s\n",
					filepath, final_path);

		free(final_path);
		final_path = NULL;
	}
	else
		DPRINTF(2, "%s/ (unreadable)\n", filepath);

	return 0;
}

int
main(int argc, char **argv)
{
	int c, ret;
	size_t len;
	while ((c = getopt(argc, argv, "dhf:")) != -1) {
		switch (c) {
		case 'f':
			FMT = calloc(strlen(optarg), sizeof(char));
			memcpy(FMT, optarg, strlen(optarg));
			break;
		case 'd':
			++_debug;
			break;
		case 'h':
		default:
			usage();
			free(FMT);
			return EXIT_SUCCESS;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2) {
		usage();
		ret = EXIT_FAILURE;
		goto free_fmt;
	}

	/* Invalid directory path? */
	if (argv[0] == NULL || *argv[0] == '\0') {
		usage();
		ret = EXIT_FAILURE;
		goto free_fmt;
	}

	DPRINTF(1, "Searching in '%s'\n", argv[0]);
	DPRINTF(1, "Will move in '%s'\n", argv[1]);

	DST = calloc(strlen(argv[1]), sizeof(char));
	memcpy(DST, argv[1], strlen(argv[1]));

	if (FMT == NULL) {
		FMT = calloc(strlen(DEFAULT_FMT), sizeof(char));
		memcpy(FMT, DEFAULT_FMT, strlen(DEFAULT_FMT));
	}

	if (nftw(argv[0], dispatch_entry, USE_FDS, FTW_PHYS)) {
		ret = EXIT_FAILURE;
		goto free_dst;
	}
	ret = EXIT_SUCCESS;

free_dst:
	free(DST);
free_fmt:
	free(FMT);
	return ret;

}
