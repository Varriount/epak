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
 *      Stripped defines and other stuff to make this compile.
 */

#ifndef ALLEGRO_SIMULATED_BASE_H
#define ALLEGRO_SIMULATED_BASE_H

#define INLINE
#define AL_CONST								const
#define AL_VAR(type, name)						extern type name
#define AL_ARRAY(type, name)					extern type name[]
#define AL_FUNC(type, name, args)				type name args
#define AL_PRINTFUNC(type, name, args, a, b)	AL_FUNC(type, name, args)
#define AL_METHOD(type, name, args)				type (*name) args
#define AL_FUNCPTR(type, name, args)			extern type (*name) args

#define _AL_MALLOC_ATOMIC		malloc
#define _AL_MALLOC				malloc
#define _AL_FREE				free
#define AL_ASSERT				assert

#ifndef EOF
	#define EOF    (-1)
#endif

#ifndef TRUE 
	#define TRUE         -1
	#define FALSE        0
#endif

#ifndef AL_MIN
	#define	AL_MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef O_BINARY
	#define O_BINARY	0
#endif

#define _al_open(filename, mode, perm)   open(filename, mode, perm)

#define PACKFILE_FLAG_WRITE      1     /* the file is being written */
#define PACKFILE_FLAG_PACK       2     /* data is compressed */
#define PACKFILE_FLAG_CHUNK      4     /* file is a sub-chunk */
#define PACKFILE_FLAG_EOF        8     /* reached the end-of-file */
#define PACKFILE_FLAG_ERROR      16    /* an error has occurred */
#define PACKFILE_FLAG_OLD_CRYPT  32    /* backward compatibility mode */
#define PACKFILE_FLAG_EXEDAT     64    /* reading from our executable */

#define ALLEGRO_NO_STRICMP 1
#define ALLEGRO_NO_STRUPR 1

//#define ALLEGRO_NO_STRDUP 1
#ifdef __cplusplus
extern "C"
#endif
char *strdup(const char *);

#ifndef AL_INLINE
	#define AL_INLINE(type, name, args, code)    static type name args code
#endif

typedef struct PACKFILE_VTABLE PACKFILE_VTABLE;
typedef struct PACKFILE PACKFILE;
typedef struct LZSS_PACK_DATA LZSS_PACK_DATA;
typedef struct LZSS_UNPACK_DATA LZSS_UNPACK_DATA;


#endif // ALLEGRO_SIMULATED_BASE_H
