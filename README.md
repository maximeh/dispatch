dispatch
========

dispatch is a command line (only, for now) program to organize your music files
by their tags.

The usage is pretty simple and self explanatory :

```bash
dispatch [-h] [-v] [-m] [-f FORMAT] SOURCE DESTINATION
        -h              This usage statement
        -v              Verbose output
        -m              Use this option to move your files to DESTINATION
                        instead of copying them (default)
        -f              Define a path format (only the following tag are supported)
                        Example : %artist/%album - %year/%track - %title
```

Given a SOURCE folder, it will analyze the id3tags of each music files and
create a directory arborescence corresponding to FORMAT.
This arborescence will be created in DESTINATION, the files will then be copied
(default) or moved.

Note: The last part of FORMAT, here %track - $title, is the filename, the
extension will be added automatically.

dependencies
------------

You will need taglib and taglib_c (taglib C bindings).
In some distro, taglib is called libtag.

build
-----

This projects uses the autotools, so you should be able to do a famous:

```bash
./configure && make
```

This should work on Linux and Mac, maybe on Windows, I haven't tested it.

why ?
-----

I was tired to use some big bloated software to do this simple task for me and
I was looking for an excuse to do some C and learn a bit about the autotools.

Maybe, I'll add a Qt GUI over this to learn a bit more.

