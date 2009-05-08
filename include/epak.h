// vim:tabstop=4 shiftwidth=4 encoding=utf-8

/** \mainpage
 * File compression routines.
 *
 * The code in this mini-library has been ripped off from Allegro's
 * 4.2.2 source code, which is gift-ware. See it's source license. The code
 * has been adapted and bloat removed. Mainly, pack_* functions are present
 * supporting basic XOR encryption and quick LZSS compression. Original
 * documentation was merged into the source code for Doxygenation, so ignore
 * any Allegro stale references in the docs.
 *
 * For a full definition of pack_* functions, see file.h. File lzss.h doesn't
 * really provide interesting functions, those are used internally by the
 * packfile functions, so you can skip it.
 *
 * Basically, a PACKFILE is similar to a stdio.h FILE structure, with three
 * main advantages:
 * - You can use chunks, which are nested files.
 * - You can compress your data. Compression favours read/write speed.
 * - You can encrypt your data with a basic XOR string. But no chaining mode.
 *
 * The password you apply is a global, you set it before writing/reading and
 * you can forget about it. Use the function packfile_password().
 *
 * Chunks are handled with pack_fopen_chunk() and pack_fclose_chunk(), but
 * first you have to open a normal PACKFILE with pack_fclose(). Note that
 * pack_fclose() is able to close hierarchies of nested files.
 *
 * There are many pack_* functions like pack_getc()/::pack_putc() to write
 * individual bytes and stuff. For huge blocks you will prefer pack_fwrite()
 * and pack_fread().
 */


#ifndef __EPAK_H__
#define __EPAK_H__

#include "epak/file.h"

#endif // __EPAK_H__
