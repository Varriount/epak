Epak file compression routines
==============================

File compression routines implemented in C, adapted to compile for iOS as a
static library, with bindings for [Nimrod](http://nimrod-code.org). The code in
this mini-library has been ripped off from [Allegro's 4.2.2 source
code](http://alleg.sourceforge.net/), [which is
gift-ware](http://alleg.sourceforge.net/license.html). Allegro has advanced in
the meantime, so you may want to check the new 5 version, or se an alternative
library without dependencies like [libtpl](http://tpl.sourceforge.net). I can't
even remember why I named it epak, maybe a mixture of [Electric
Hands](http://elhaso.com/) + **pa**[c]**k**files?

The code has been adapted and bloat removed. Mainly, pack_* functions are
present supporting basic XOR encryption and quick LZSS compression. [Original
documentation](http://alleg.sourceforge.net/stabledocs/en/alleg030.html) was
merged into the source code for Doxygenation, so ignore any Allegro stale
references in the docs.

For a full definition of pack_* functions, see
[file.h](https://github.com/gradha/epak/blob/master/include/epak/file.h). File
[lzss.h](https://github.com/gradha/epak/blob/master/include/epak/lzss.h)
doesn't really provide interesting functions, those are used internally by the
packfile functions, so you can skip it.

Basically, a
[PACKFILE](http://alleg.sourceforge.net/stabledocs/en/alleg001.html#PACKFILE)
is similar to a stdio.h FILE structure, with three main advantages:

* You can use chunks, which are nested files.
* You can compress your data. Compression favours read/write speed.
* You can encrypt your data with a basic XOR string. But no chaining mode.

The password you apply is a global, you set it before writing/reading and you
can forget about it. Use the function
[packfile_password()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#packfile_password).

Chunks are handled with
[pack_fopen_chunk()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_fopen_chunk)
and
[pack_fclose_chunk()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_fclose_chunk),
but first you have to open a normal PACKFILE with
[pack_fopen()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_fopen).
Note that
[pack_fopen()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_fopen)
is able to close hierarchies of nested files.

There are many pack_* functions like
[pack_getc()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_getc)/[pack_putc()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_putc)
to write individual bytes and stuff. For huge blocks you will prefer
[pack_fwrite()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_fwrite)
and
[pack_fread()](http://alleg.sourceforge.net/stabledocs/en/alleg030.html#pack_fread).


Installation/Usage
==================

The project comes with a [GNU Makefile](http://www.gnu.org/software/make/). In
it's basic form you can type ``make install`` and it should build the static
library with Xcode for iOS, leaving it in the ``build`` subdirectory, and will
also build and install the normal C library into ``/usr/local/lib`` which you
can use in other C code, most likely the provided bindings.

This library is very small so there was no point in compiling stuff
dynamically. You link to it statically passing ``-lepak`` to your commandline
(and likely ``-L/usr/local/lib`` too).

The [example](https://github.com/gradha/epak/tree/master/example) subdirectory
contains a C example progam using the library along and it's pythonic version.


Python bindings
===============

Part of the reason of using these functions was to implement [the iOS game
Submarine
Hunt](https://itunes.apple.com/en/app/submarine-hunt/id325946564?mt=8) so as to
pack graphical data as tightly and efficiently as possible. So I wrote a python
binding and developed a toolchain to arrange and pack resources to my liking.

You can find *rests* of this code in the python subdirectory. However, python
proved to be a really bad choice: it wasn't easy at all to interface with C,
and every time I updated the python interpreter or operative system everything
broke and required little tweaks. Recently I've upgraded to MacOSX Lion and the
stuff compiles, but every time you run it you get a nice traceback call stack
of python crashing during calls to the C module. Nice, isn't it?

So much for *glue code*, you end up *glued forever to maintenance of a brittle
language*. Yay. So the python code is not maintained any more in favour of
[Nimrod](http://nimrod-code.org). You can keep it and fix it if you want to.


Nimrod bindings
===============

[Nimrod](http://nimrod-code.org) code is contained within the
[nimrod](https://github.com/gradha/epak/tree/master/nimrod) subdirectory, which
has the raw interface plus an object oriented interface inspired on the python
bindings. Another
[tests](https://github.com/gradha/epak/tree/master/nimrod/tests) subdirectory
highlights how these modules can be used.

Note that using the nimrod interface will implicitly to link your program
against the C epak library, so you need to install that on your system or
compilation of your nimrod program will fail. Additional parameters for
compilation are contained inside [nimrod configuration
files](https://github.com/gradha/epak/blob/master/nimrod/tests/nimrod.cfg), so
you can type ``nimrod c -r testnimepakoo.nim`` and everything should work as
long as you have previously installed the C library on your system.


Documentation
=============

Basic documentation was ripped from Allegro but didn't make it through all the
bindings. You can build it with [Doxygen](http://www.doxygen.org). In fact, the
normal makefile will try to build it but won't fail with an error if it can't.
In these situations use
[http://alleg.sourceforge.net/stabledocs/en/alleg030.html](http://alleg.sourceforge.net/stabledocs/en/alleg030.html)
as the authoritative reference, or read the source, it's not really hard and
the number of functions is small.

Good luck!
