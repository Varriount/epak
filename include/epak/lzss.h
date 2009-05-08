// vim:tabstop=4 shiftwidth=4 encoding=utf-8
/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Compression routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_LZSS_H
#define ALLEGRO_LZSS_H

#include "epak/base.h"

#ifdef __cplusplus
	extern "C" {
#endif

AL_FUNC(LZSS_PACK_DATA *, create_lzss_pack_data, (void));
AL_FUNC(void, free_lzss_pack_data, (LZSS_PACK_DATA *dat));
AL_FUNC(int, lzss_write, (PACKFILE *file, LZSS_PACK_DATA *dat, int size, unsigned char *buf, int last));

AL_FUNC(LZSS_UNPACK_DATA *, create_lzss_unpack_data, (void));
AL_FUNC(void, free_lzss_unpack_data, (LZSS_UNPACK_DATA *dat));
AL_FUNC(int, lzss_read, (PACKFILE *file, LZSS_UNPACK_DATA *dat, int s, unsigned char *buf));
AL_FUNC(int, _al_lzss_incomplete_state, (AL_CONST LZSS_UNPACK_DATA *dat));


#ifdef __cplusplus
	}
#endif

#endif          /* ifndef ALLEGRO_LZSS_H */


