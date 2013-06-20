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

#pragma once

#define _XOPEN_SOURCE 600
#define log(format, ...) if (verbose == 1) fprintf (stderr, format, ## __VA_ARGS__)

#if _APPLE
    #include <copyfile.h>
#endif
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <ftw.h>
#include <getopt.h>
#include <libgen.h>
#ifdef _LINUX
    #include <linux/limits.h>
#elif _APPLE
    #include <limits.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _LINUX
    #include <sys/sendfile.h>
#endif
#include <taglib/tag_c.h>
#include <unistd.h>

static int verbose = 0, copy = 1;
static char source_path[PATH_MAX];
static char dest_path[PATH_MAX];
static char *format = "%artist/%album/%track - %title";
struct tag{
    const char *place_holder;
    const char *value;
    struct tag *next;
};

static void _copy (const char *src, const char *dest);
static void _mkdir (const char *path);
static void append_tag (struct tag *llist, const char *holder, const char
        *value);
static char *build_path_from_tag (const char *path, const char *ext);
static int dispatch (const char *name, const struct stat *status, int type);
static inline void escape (char *source);
static void free_list (struct tag *head);
static char *get_ext (const char *filename);
static inline int str_compare(const char* a, const char* b);
static char *str_replace (const char *orig, const char *rep, const char *with);
static inline void usage ();

