dispatch
========

dispatch is a command line program to organize your music files by their
metadata.

The usage is pretty simple and self explanatory :

```bash
# ./dispatch -h
dispatch [-h] [-d] [-f FORMAT] SOURCE DEST
	-h		This usage statement
	-d		Debug output (-dd for more, ...)
	-f		Path format - Tag supported:
				- %a (artist)
				- %A (album)
				- %y (year)
				- %t (track)
				- %T (title)
```

It'll descends recursively into SOURCE, find every mp3, m4a, flac and ogg
files, it'll try to read the id3tags and create a path corresponding to FORMAT.
The path will be created and the file be copied there.

The default FORMAT if unspecified is "%a/%A/%t - %T".
For example with the song "AC/DC - Best of AC/DC - 01 Black In Black.mp3",
it'll create "AC_DC/Best Of AC_DC/01 - Black In Black.mp3".

Note: that the '/' are replaced by '_' and the extension will be added
automatically.

dependencies
------------

You will need taglib and taglib_c (taglib C bindings).
In some distro, taglib is called libtag.

build
-----

This projects uses the autotools, so you should be able to do a famous:

```bash
./bootstrap.sh && ./configure && make
```

why ?
-----

Why not ? It's just a small silly tool that I use to organize my music folder,
within scripts, it's only used to have some fun around code, so sometimes,
stuff may be over engineered, but hey, that's the fun part !

