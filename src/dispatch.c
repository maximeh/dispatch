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

#include "dispatch.h"

int
main (int argc, char **argv)
{
	int c, exit_code = EXIT_SUCCESS;
	while ((c = getopt (argc, argv, "hf:vm")) != -1)
	{
		switch (c)
		{
			case 'f':
				/* format string specified */
				format = optarg;
				break;
			case 'v':
				/* verbose mode */
				verbose = 1;
				break;
			case 'm':
				/* move instead of copy */
				copy = 0;
				break;
			case 'h':
			default:
				usage ();
				exit (EXIT_SUCCESS);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
	{
		usage ();
		exit (EXIT_SUCCESS);
	}

	source_path = strndup (argv[0], PATH_MAX);
	if (source_path == NULL)
		err (EXIT_FAILURE, "ERROR: Could not copy %s into 'source_path'\n", argv[0]);

	dest_path = strndup (argv[1], PATH_MAX);
	if (dest_path == NULL)
	{
		free (source_path);
		err (EXIT_FAILURE, "ERROR: Could not copy %s into 'dest_path'\n", argv[1]);
	}

	/* List directory recursively */
	log ("Searching in '%s'\n", source_path);
	log ("Will move in '%s'\n", dest_path);
	if (ftw (source_path, dispatch, 20) == -1)
		perror ("ftw");

	free (source_path);
	free (dest_path);
	exit (exit_code);
}

int
dispatch (const char *path, const struct stat *status, int type)
{
	char *formated_dir_path = NULL;
	char *formated_path = NULL;
	char *formated_path_copy = NULL;
	char *ext = NULL;

	/* We only want to treat regular file */
	/* We can't return non zero here, otherwise ftw, our caller, would exit */
	if (type == FTW_NS || type != FTW_F)
		return 0;

	log ("\nCurrent path is: '%s'\n", path);

	/* Test if the file is valid */
	/* we got the extension of each file */
	if ((ext = get_ext (path)) == NULL)
	{
		log ("%s doesn't have a valid extension.\n", path);
		/* Again, we can't return -1 or the whole listing will stop */
		return 0;
	}
	log ("Extension: '%s'\n", ext);

	/* Grab id3tag for each filename  */
	if ((formated_path = build_path_from_tag (path, ext)) == NULL)
	{
		fprintf (stderr, "ERROR: Could not read tag.\n");
		return -1;
	};
	log ("Formated path is: %s\n", formated_path);

	if ((formated_path_copy = strdup (formated_path)) == NULL)
	{
		fprintf (stderr, "ERROR: Could not duplicate %s.\n", formated_path);
		goto free_exit;
	}

	formated_dir_path = dirname (formated_path_copy);
	if (formated_dir_path == NULL)
	{
		fprintf (stderr, "ERROR: Could not obtain 'dirname' of %s.\n", path);
		goto free_exit;
	}
	log ("dirname: '%s'\n", formated_dir_path);

	/* Create our dir if necessary */
	log ("Creating tree : '%s'\n", formated_dir_path);
	if (_mkdirs (formated_dir_path) == -1)
	{
		fprintf (stderr, "ERROR: '%s' already exists.\n", formated_dir_path);
		goto free_exit;
	}

	/* cp or move file to full path */
	/* If we are asked to move, try to rename first, if it doesn't work, try to
	 * copy then delete. */
	if (copy == 1)
	{
		_copy (path, formated_path);
	}
	else if (rename (path, formated_path))
	{
		_copy (path, formated_path);
		remove (path);
	}

	free (formated_path);
	free (formated_path_copy);
	return 0;

free_exit:
	free (formated_path);
	free (formated_path_copy);
	return -1;
}

static char *
build_path_from_tag (const char *file_path, const char *ext)
{
	struct tag *tag_list = NULL, *head = NULL;
	char *full_formated_path = NULL;
	char * formated_path = NULL;
	char * temp_path = NULL;
	char track_str[3];
	char year_str[5];
	int track;
	TagLib_File *file;
	TagLib_Tag *tag;

	if ((file = taglib_file_new (file_path)) == NULL)
	{
		fprintf (stderr, "ERROR: The type of %s cannot be determined or the file "
				"cannot be opened\n", file_path);
		return NULL;
	}

	if (!taglib_file_is_valid (file))
	{
		fprintf (stderr, "ERROR: The file %s is not valid\n", file_path);
		goto free_taglib;
	}

	tag = taglib_file_tag (file);
	if (tag == NULL)
	{
		fprintf (stderr, "ERROR: The file %s is not valid.\n", file_path);
		goto free_taglib;
	}

	/* Create formated_path using tag info of file_path */
	formated_path = strdup (format);
	if (formated_path == NULL)
	{
		fprintf (stderr, "ERROR: Could not duplicate the string '%s'.\n", file_path);
		goto free_taglib;
	}

	tag_list = calloc (1, sizeof (struct tag));
	if (tag_list == NULL)
	{
		fprintf (stderr, "ERROR: Could not malloc 'tag_list'.\n");
		goto free_taglib;
	}
	/* Create root element of the tag_list */
	tag_list->place_holder = "%album";
	tag_list->value = taglib_tag_album (tag);
	tag_list->next = NULL;
	log ("album\t-\t\"%s\"\n", taglib_tag_album (tag));

	// Keep a track of the head
	head = tag_list;

	tag_list = append_tag (tag_list, "%artist", taglib_tag_artist (tag));
	if (tag_list == NULL)
	{
		fprintf (stderr, "ERROR: Could not append '%%artist' to 'tag_list'.\n");
		goto free_taglib;
	}
	log ("artist\t-\t\"%s\"\n", taglib_tag_artist (tag));

	tag_list = append_tag (tag_list, "%title", taglib_tag_title (tag));
	if (tag_list == NULL)
	{
		fprintf (stderr, "ERROR: Could not append '%%title' to 'tag_list'.\n");
		goto free_taglib;
	}
	log ("title\t-\t\"%s\"\n", taglib_tag_title (tag));

	track = taglib_tag_track (tag);
	log ("track\t-\t\"%02i\"\n", track);

	if (sprintf (track_str, track < 10 ? "0%d" : "%d", track) < 0)
	{
		fprintf (stderr, "ERROR: Could not add leading '0' to %d.\n", track);
		goto free_taglib;
	}

	tag_list = append_tag (tag_list, "%track", track_str);
	if (tag_list == NULL)
	{
		fprintf (stderr, "ERROR: Could not append '%%track' to 'tag_list'.\n");
		goto free_taglib;
	}

	log ("year\t-\t\"%d\"\n", taglib_tag_year (tag));
	if (sprintf (year_str, "%d", taglib_tag_year (tag)) < 0)
	{
		fprintf (stderr, "ERROR: Could not add leading '0' to %d.\n", track);
		goto free_taglib;
	}

	tag_list = append_tag (tag_list, "%year", year_str);
	if (tag_list == NULL)
	{
		fprintf (stderr, "ERROR: Could not append '%%year' to 'tag_list'\n");
		goto free_taglib;
	}

	/* Rewind to the head */
	tag_list = head;

	/* Loop through the tag_list */
	while (tag_list->next != NULL) {
		log ("Search %s => %s\n", tag_list->place_holder, tag_list->value);

		/* Replace place_holder with its value in formated_path */
		temp_path = str_replace (formated_path, tag_list->place_holder,
				tag_list->value);
		if (temp_path == NULL)
		{
			fprintf (stderr, "ERROR: str_replace encountered an error.\n");
			goto free_taglib;
		}
		/* The formated_path pointer will be overwritten by the next
		 * operation we must free it before or it will be lost. */
		free (formated_path);

		/* formated_path is "updated" with the new value */
		formated_path = strdup (temp_path);

		/* Since temp_path is allocated by str_replace, it must be freed */
		free (temp_path);

		log ("actual path - \"%s\"\n", formated_path);
		tag_list = tag_list->next;
	}

	/* Free the tags and list we just used */
	free_list (head);
	taglib_tag_free_strings ();
	taglib_file_free (file);

	/* Assemble the wanted full formatted path */
	/* This is where the file should be moved to */
	full_formated_path = malloc (((strlen (dest_path) + strlen (formated_path) +
					strlen (ext)) * sizeof (char)) + 3);
	if (full_formated_path == NULL)
	{
		fprintf (stderr, "ERROR: Could not malloc 'full_formated_path'.\n");
		goto free_formated_path;
	}

	if ( sprintf (full_formated_path, "%s/%s.%s", dest_path, formated_path, ext) < 0)
	{
		fprintf (stderr, "ERROR: Could not create 'full_formated_path'.\n");
		goto free_formated_path;
	}

	log ("Final path is: %s\n", full_formated_path);
	free (formated_path);

	return full_formated_path;

free_formated_path:
		free (formated_path);
		return NULL;

free_taglib:
	taglib_file_free (file);
	return NULL;

}

static int
_mkdirs (const char *path)
{
	// Path MUST not be terminated by a '/'
	// It should never happen as path came here from a dirname call
	// which strips the last '/'.
	char *opath;
	char *p;
	size_t len;
	len = strlen (path);

	opath = malloc (len + 1);
	if (opath == NULL)
	{
		perror ("_mkdirs");
		return -1;
	}
	strncpy (opath, path, len + 1);

	// you want to avoid the first /
	for(p = opath + 1; *p; ++p)
		if(*p == '/') {
			*p = '\0';
			if (_mkdir (opath) == -1)
			{
				free (opath);
				return -1;
			}
			*p = '/';
		}
	if (_mkdir (opath) == -1)
	{
		free (opath);
		return -1;
	}

	free (opath);
	return 0;
}

static int
_mkdir (const char *path)
{
	struct stat path_stat;
	int error = stat (path, &path_stat);

	if (!error && !S_ISDIR(path_stat.st_mode))
	{
		fprintf (stderr, "ERROR: %s is not a directory.\n", path);
		return -1;
	}

	if (error == -1)
	{
		if (ENOENT != errno)
			goto error_mkdir;
		if (mkdir (path, S_IRWXU) != 0)
			goto error_mkdir;
	}
	return 0;

error_mkdir:
	perror("_mkdir");
	return -1;

}

static int
_copy (const char *src, const char *dest)
{
	int err = 0;
	FILE *in_fd, *out_fd;
	size_t bytes_left;
	struct stat in_st;

	in_fd = fopen (src, "rb");
	if (in_fd == NULL)
	{
		perror ("_copy");
		return 1;
	}

	if (fstat(fileno (in_fd), &in_st) != 0)
	{
		fclose (in_fd);
		perror ("_copy");
		return 1;
	}
	bytes_left = in_st.st_size;

	out_fd = fopen (dest, "w+b");
	if (out_fd == NULL)
	{
		fclose (in_fd);
		perror ("_copy");
		return 1;
	}

#ifdef _LINUX
	size_t bytes_copied;

	if ((err = posix_fadvise (fileno (in_fd), 0, 0, POSIX_FADV_SEQUENTIAL)) != 0)
	{
		fprintf (stderr, "ERROR: %s\n", strerror (err));
		goto close_fds;
	}

	if ((err = posix_fallocate (fileno (out_fd), 0, bytes_left)) != 0)
	{
		fprintf (stderr, "ERROR: %s\n", strerror (err));
		goto close_fds;
	}

	bytes_copied = sendfile (fileno (out_fd), fileno (in_fd), 0, bytes_left);
	if (bytes_copied != bytes_left)
		goto close_fds;
#elif _APPLE
	/* Copy the data, extended attributes and ACL */
	err = fcopyfile (fileno (in_fd), fileno (out_fd), NULL, COPYFILE_DATA
			| COPYFILE_XATTR | COPYFILE_ACL);
	if (err < 0)
		goto close_fds;
#else
	// Copy buffer by buffer.

	size_t buf_size = BUFFER_SIZE;
	if (BUFFER_SIZE > bytes_left)
		buf_size = bytes_left;
	char buf[buf_size];

	// Our buffer is as large as the size of the whole chunk we want to copy
	// Do it directly, no need to buffer this.
	if (buf_size == bytes_left)
	{
		if (fread (buf, bytes_left, 1, in_fd) != 1)
		{
			fprintf (stderr, "ERROR: Could not read %zu bytes.\n", bytes_left);
			goto close_fds;
		}
		if (fwrite (buf, bytes_left, 1, out_fd) != 1)
		{
			fprintf (stderr, "ERROR: Could not write %zu bytes.\n", bytes_left);
			goto close_fds;
		}
		return 0;
	}

	while (fread (buf, 1, buf_size, in_fd) == buf_size) {
		if (fwrite (buf, 1, buf_size, out_fd) != buf_size)
		{
			fprintf (stderr, "ERROR: Could not write chunk of %zu bytes.\n", buf_size);
			goto close_fds;
		}
		bytes_left -= buf_size;
	}

	// Write last buffer
	if (fread (buf, 1, bytes_left, in_fd))
	{
		fprintf (stderr, "ERROR: Could not read last chunk of %zu bytes.\n",
				bytes_left);
		goto close_fds;
	}
	if (fwrite (buf, 1, bytes_left, out_fd) != bytes_left)
	{
		fprintf (stderr, "ERROR: Could not write last chunk of %zu bytes.\n",
				bytes_left);
		goto close_fds;
	}
#endif

	fclose (in_fd);
	fclose (out_fd);
	return 0;

close_fds:
	fclose (in_fd);
	fclose (out_fd);
	perror ("_copy");
	return 1;
}

static char *
get_ext (const char *filename)
{
	/* Find the extension */
	char *ext = strrchr (filename, '.');
	if (ext == NULL)
		return NULL;

	/* Avoid the dot */
	char *ext_dup = ++ext;

	for (;*ext; ++ext)
		*ext = tolower(*ext);

	if (0 != str_compare (ext_dup, "mp3") &&
			0 != str_compare (ext_dup, "m4a") &&
			0 != str_compare (ext_dup, "flac") &&
			0 != str_compare (ext_dup, "ogg"))
		return NULL;

	return ext_dup;
}

// Stolen from Jason Mooberry (@jasonmoo on GitHub)
static int
str_compare(const char* a, const char* b)
{
	for(;*a == *b; ++a, ++b)
		if (*a == '\0' || *b == '\0') return 0;
	return *a - *b;
}

static char *
str_replace(const char *orig, const char *rep, const char *with)
{
	char *result; // the return string
	char *ins;    // the next insert point
	char *tmp;    // varies
	int len_orig; // length of origin
	int len_rep;  // length of rep
	int len_with; // length of with
	int len_front;// distance between rep and end of last rep
	int count;    // number of replacements

	if (!orig)
	{
		fprintf (stderr, "ERROR: There is no string to look at.\n");
		return NULL;
	}

	if (!rep || !(len_rep = strlen (rep)))
	{
		fprintf (stderr, "ERROR: There is no replacement string.\n");
		return NULL;
	}

	if (!(ins = strstr (orig, rep)))
	{
		fprintf (stderr, "ERROR: %s doesn't exists in %s.\n", rep, orig);
		return NULL;
	}

	len_orig = strlen (orig);

	char *with_cpy = strdup (with);
	if (with_cpy == NULL)
	{
		fprintf (stderr, "ERROR: Could not strdup 'with_cpy'.\n");
		return NULL;
	}

	// We don't need to espace if '/' is not present in with_cpy
	if (strchr (with_cpy, '/') != NULL) escape (with_cpy);
	len_with = strlen (with_cpy);

	// Find out how many times rep exists in orig.
	// At this moment ins point to the first occurrence of rep in orig.
	for (count = 0; (tmp = strstr (ins, rep)); ++count)
		ins = tmp + len_rep;

	// first time through the loop, all the variable are set correctly
	// from here on,
	//    tmp points to the end of the result string
	//    ins points to the next occurrence of rep in orig
	//    orig points to the remainder of orig after "end of rep"
	tmp = result = malloc (len_orig + (len_with - len_rep) * count + 1);
	if (!result || !tmp)
	{
		free (with_cpy);
		return NULL;
	}

	/* Replace all the occurences */
	while (count--) {
		ins = strstr (orig, rep);
		len_front = ins - orig;

		// Copy up to the first rep to tmp, and make tmp point to its own end.
		tmp = strncpy (tmp, orig, len_front) + len_front;

		/* Append to tmp the string to replace with, and make tmp point to its
			 own end. */
		tmp = strncpy (tmp, with_cpy, len_with) + len_with;

		orig += len_front + len_rep; // move to next "end of rep"
	}

	/* Copy from the last "end of rep" to the end of orig into tmp */
	strcpy (tmp, orig);

	free (with_cpy);
	return result;
}

static void
escape (char *source)
{
	while (*source != '\0')
	{
		if (*source == '/')
			*source = '_';
		++source;
	}
}

static struct tag *
append_tag (struct tag *llist, const char *place_holder, const char *value)
{
	/* Create a new element */
	struct tag *new = malloc (sizeof (struct tag));
	if (new == NULL)
	{
		fprintf (stderr, "ERROR: Could not malloc tag 'new'.\n");
		return NULL;
	}

	new->place_holder = place_holder;
	new->value = value;
	new->next = NULL;

	/* Put the new element at the end of the list */
	llist->next = new;

	/* Return the new end of the list */
	return llist->next;
}

static void
free_list (struct tag *head)
{
	struct tag* tmp;

	/* Loop through the list to free each tag */
	while (head != NULL)
	{
		log ("Freeing %s => %s\n", head->place_holder, head->value);
		/* Keep current element */
		tmp = head;
		/* Move the head to the next element */
		head = head->next;
		/* Free current element */
		free (tmp);
	}
}

static void
usage (void)
{
	(void) fprintf (stdout, "dispatch [-h] [-v] [-m] [-f FORMAT] SOURCE DEST\n");
	fputs(
			"\t-h\t\tThis usage statement\n"
			"\t-v\t\tVerbose output\n"
			"\t-m\t\tUse this option to move your files to DESTINATION\n"
			"\t\t\tinstead of copying them (default)\n"
			"\t-f\t\tDefine a path format (only the following tag are supported)\n"
			"\t\t\tExample : %artist/%album - %year/%track - %title\n"
			, stdout);
}
