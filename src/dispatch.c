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

    /* source folder */
    if ( realpath (argv[0], source_path) == NULL )
        perror ("realpath input");

    /* destination folder */
    if ( realpath (argv[1], dest_path) == NULL )
        perror ("realpath output");

    /* List directory recursively */
    log ("Searching in %s \n", source_path);
    log ("Will move in %s\n", dest_path);
    if (ftw (source_path, dispatch, 20) == -1)
        perror ("ftw");

    fclose ( stdin );
    fclose ( stdout );
    fclose ( stderr );

    exit (exit_code);
}

int
dispatch (const char *path, const struct stat *status, int type)
{
    char *formated_dir_path = NULL;
    char *formated_path = NULL;
    char *formated_path_copy = NULL;
    char *ext = NULL;
    struct stat dir_stat;

    /* We only want to treat regular file */
    if (type == FTW_NS || type != FTW_F)
        return 0;

    log ("\nCurrent path is: %s\n", path);

    /* Test if the file is valid */
    /* we got the extension of each file */
    if ((ext = get_ext (path)) == NULL)
    {
        log ("No extension for %s\n", path);
        return 0;
    }
    log ("Extension: %s\n", ext);

    /* Grab id3tag for each filename  */
    if ((formated_path = build_path_from_tag (path, ext)) == NULL)
    {
        log ("Error reading tag\n");
        return 0;
    };
    log ("Formated path is: %s\n", formated_path);

    formated_path_copy = strdup (formated_path);
    formated_dir_path = dirname (formated_path_copy);
    if (formated_dir_path == NULL)
    {
        log ("dirname: %s\n", path);
        goto clean_exit;
    }
    log ("Dirname: %s\n", formated_dir_path);

    /* Create our dir if necessary */
    if (stat (formated_dir_path, &dir_stat) == -1)
    {
        log ("Creating tree : %s\n", formated_dir_path);
        _mkdir (formated_dir_path);
    }
    else if (!S_ISDIR (dir_stat.st_mode))
    {
        /* S_ISDIR is a macro */
        log ("%s exists and it's not a dir.\n", formated_dir_path);
        goto clean_exit;
    }

    /* cp or move file to full path */
    /* If we are asked to move, try to rename first, if it doesn't work. Try to
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
    goto clean_exit;

clean_exit:
    free (formated_path);
    free (formated_path_copy);
    return 0;
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
        log ("The type of %s cannot be determined or the file cannot "
               "be opened\n", file_path);
        return NULL;
    }

    if (!taglib_file_is_valid (file))
    {
        log ("The file %s is not valid\n", file_path);
        goto free_taglib;
    }

    tag = taglib_file_tag (file);
    if (tag == NULL)
    {
        log ("The file %s is not valid", file_path);
        goto free_taglib;
    }

    /* Create formated_path using tag info of file_path */
    formated_path = strdup (format);

    tag_list = malloc (sizeof (struct tag));
    /* Create root element of the tag_list */
    tag_list->place_holder = "%album";
    tag_list->value = taglib_tag_album (tag);
    tag_list->next = NULL;
    log ("album\t-\t\"%s\"\n", taglib_tag_album (tag));

    // Keep a track of the head
    head = tag_list;

    tag_list = append_tag (tag_list, "%artist", taglib_tag_artist (tag));
    log ("artist\t-\t\"%s\"\n", taglib_tag_artist (tag));

    tag_list = append_tag (tag_list, "%title", taglib_tag_title (tag));
    log ("title\t-\t\"%s\"\n", taglib_tag_title (tag));

    track = taglib_tag_track (tag);
    log ("track\t-\t\"%02i\"\n", track);
    sprintf (track_str, track < 10 ? "0%d" : "%d", track);
    tag_list = append_tag (tag_list, "%track", track_str);

    log ("year\t-\t\"%d\"\n", taglib_tag_year (tag));
    sprintf (year_str, "%d", taglib_tag_year (tag));
    tag_list = append_tag (tag_list, "%year", year_str);

    /* Rewind to the head */
    tag_list = head;

    /* Loop through the tag_list */
    while (tag_list->next != NULL) {
        log ("Search %s => %s\n", tag_list->place_holder, tag_list->value);

        /* Replace place_holder with its value in formated_path */
        temp_path = str_replace (formated_path, tag_list->place_holder,
                tag_list->value);
        if (temp_path != NULL)
        {
            /* The formated_path pointer will be overwritten by the next
             * operation we must free it before or it will be lost. */
            free (formated_path);
            /* formated_path is "updated" with the new value */
            formated_path = strdup (temp_path);
            /* Since temp_path is allocated by str_replace, it must be freed */
            free (temp_path);
        }
        log ("actual path - \"%s\"\n", formated_path);
        tag_list = tag_list->next;
    }

    /* Free the tags and list we just used */
    free_list (head);
    taglib_tag_free_strings ();
    taglib_file_free (file);

    /* Assemble the wanted full formatted path */
    /* This is where the file should be moved to */
    full_formated_path = malloc (strlen (dest_path) + strlen (formated_path)
            + strlen (ext) + 3);
    sprintf (full_formated_path, "%s/%s.%s", dest_path, formated_path, ext);
    log ("Final path is: %s\n", full_formated_path);
    free (formated_path);

    return full_formated_path;

free_taglib:
    taglib_file_free (file);
    return NULL;

}

static void
_mkdir (const char *path)
{
    // Path MUST not be terminated by a '/'
    // It should never happen as path came here from a dirname call
    // which strips the last '/'.
    char *opath;
    char *p;
    size_t len;
    len = strlen (path);

    opath = malloc (len + 1);
    if (!opath)
    {
        perror ("_mkdir");
        return;
    }
    strncpy (opath, path, len + 1);

    for(p = opath; *p; ++p)
        if(*p == '/') {
            *p = '\0';
            if(access (opath, F_OK))
                mkdir (opath, S_IRWXU);
            *p = '/';
        }
    if(access (opath, F_OK))         /* if path is not terminated with / */
        mkdir (opath, S_IRWXU);

    free (opath);
}

static void
_copy (const char *src, const char *dest)
{
    int err = 0;
    FILE *in_fd, *out_fd;
    struct stat in_st;

    in_fd = fopen (src, "rb");
    if (in_fd == NULL)
        perror ("_copy src");

    out_fd = fopen (dest, "w+b");
    if (out_fd == NULL)
        perror ("_copy dest");

    fstat(fileno (in_fd), &in_st);

    #ifdef _LINUX
        size_t bytes_copied;

        if ((err = posix_fadvise (fileno (in_fd), 0, 0, POSIX_FADV_SEQUENTIAL)) != 0)
        {
            fprintf (stderr, "%s\n", strerror (err));
            goto close_fds;
        }

        if ((err = posix_fallocate (fileno (out_fd), 0, in_st.st_size)) != 0)
        {
            fprintf (stderr, "%s\n", strerror (err));
            goto close_fds;
        }

        bytes_copied = sendfile (fileno (out_fd), fileno (in_fd), 0,
                in_st.st_size);
        if (bytes_copied != in_st.st_size)
        {
            perror ("sendfile");
            return;
        }
    #elif _APPLE
        /* Copy the data, extended attributes and ACL */
        err = fcopyfile (fileno (in_fd), fileno (out_fd), NULL, COPYFILE_DATA
                | COPYFILE_XATTR | COPYFILE_ACL);
        if (err < 0)
        {
            perror ("copyfile");
            return;
        }
    #else
        // Copy buffer by buffer.
        // The buffer is only 16KB to stay in the L1 cache of the processor
        const int BUF_SIZE = 16384;
        char buf[BUF_SIZE];
        // Keep a copy of the file size
        size_t bytes_left = in_st.st_size;
        size_t size;

        while ((size = fread(buf, sizeof (buf), 1, in_fd))) {
            if (fwrite (buf, sizeof (buf), 1, out_fd) != size)
                perror ("_copy size");
            bytes_left -= BUF_SIZE;
        }
        // Write last buffer
        fread (buf, bytes_left, 1, in_fd);
        fwrite (buf, bytes_left, 1, out_fd);
    #endif

    goto close_fds;

close_fds:
    fclose (in_fd);
    fclose (out_fd);
    return;
}

static char *
get_ext (const char *filename)
{
    /* Find the extension; the +1 let us avoid the dot. */
    char *ext = strrchr (filename, '.') + 1;
    char *ext_dup = ext;
    if (ext == NULL)
        return NULL;

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
        return NULL;
    if (!rep || !(len_rep = strlen (rep)))
        return NULL;
    if (!(ins = strstr (orig, rep)))
        return NULL;

    len_orig = strlen (orig);

    char *with_cpy = strdup (with);
    if (strchr (with_cpy, '/') != NULL) escape (with_cpy);
    len_with = strlen (with_cpy);

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
        tmp = strncpy (tmp, orig, len_front) + len_front;
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
    (void) fprintf (stderr, "dispatch [-h] [-v] [-m] [-f FORMAT] SOURCE DESTINATION\n");
    fputs(
            "\t-h\t\tThis usage statement\n"
            "\t-v\t\tVerbose output\n"
            "\t-m\t\tUse this option to move your files to DESTINATION\n"
            "\t\t\tinstead of copying them (default)\n"
            "\t-f\t\tDefine a path format (only the following tag are supported)\n"
            "\t\t\tExample : %artist/%album - %year/%track - %title\n"
            , stderr);
}
