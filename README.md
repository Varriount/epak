Epak file compression routines
==============================

File compression routines implemented in C, adapted to compile for iOS as a
static library, with bindings for [Nimrod](http://nimrod-code.org). The code in
this mini-library has been ripped off from [Allegro's 4.2.2 source
code](http://alleg.sourceforge.net/), [which is
gift-ware](http://alleg.sourceforge.net/license.html). Allegro has advanced in
the meantime, so you may want to check the new 5th version, or see an
alternative library without huge dependencies like
[libtpl](http://tpl.sourceforge.net). I can't even remember why I named it
epak, maybe a mixture of [Electric Hands](http://elhaso.es/) +
**pa**[c]**k**files?

The code has been adapted and bloat removed. Mainly, pack_* functions are
present supporting basic XOR encryption and quick LZSS compression. [Original
documentation](http://alleg.sourceforge.net/stabledocs/en/alleg030.html) was
merged into the C source code for Doxygenation, so ignore any Allegro stale
references in the generated docs.

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


License
=======

This package is provided under the same same license as the C code it was based
on. The [Allegro 4.0 giftware
license](http://alleg.sourceforge.net/license.html) is basically a MIT license.


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

The Nimrod implementation doesn't use the *normal* C version. Instead, the C
source code was concatenated into a single file to be embedded inside Nimrod
using its `{.compile.}` pragma. Thanks to this you are not required to install
the C version *previously*, just importing the epak module will be enough.

In order to use the epak module in Nimrod you will have to either checkout this
git repository and use Nimrod's ``--path`` switch to include epak's directory
in the search list for modules, or use
[Babel](https://github.com/nimrod-code/babel). If you have babel installed, you
can type:

    babel install nimepak

And now you can ``import nimepak`` or ``import nimepakoo`` in your programs and
everything should work. If you already have the git checkout and want to use
babel anyway, you can type ``babel install`` inside the checkout and it will
install the local version to your babel path.


Documentation
=============

Basic documentation was ripped from Allegro but didn't make it through all the
bindings. You can build it with [Doxygen](http://www.doxygen.org). In fact, the
normal makefile will try to build it but won't fail with an error if it can't.
In these situations use
<http://alleg.sourceforge.net/stabledocs/en/alleg030.html> as the authoritative
reference, or read the source, it's not really hard and the number of functions
is small.

The Nimrod version has a few docstrings in the nimepakoo module, which presents
an object oriented wrapper around the raw C API. You can generate an HTML of
the docstrings by going to wherever you installed the epak source and running
Nimrod's doc2 command on the source. Example:

    cd ~/.babel/libs/nimepak-1.0.0/nimrod
    nimrod doc2 nimepak.nim
    nimrod doc2 nimepakoo.nim

This will generate an html file in the same directory containing basic
information (mostly ripped from the Allegro C docs).

Good luck!
