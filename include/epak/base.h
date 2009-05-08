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


/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "mpw"
#define ALLEGRO_BIG_ENDIAN
#undef ALLEGRO_CONSOLE_OK

#define INLINE
#define ZERO_SIZE_ARRAY(type, name)             type name[64]
#define AL_CONST				const
#define AL_VAR(type, name)                      extern type name
#define AL_ARRAY(type, name)                    extern type name[]
#define AL_FUNC(type, name, args)               type name args
#define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args)
#define AL_METHOD(type, name, args)             type (*name) args
#define AL_FUNCPTR(type, name, args)            extern type (*name) args

#define _AL_MALLOC_ATOMIC			malloc
#define _AL_FREE				free
#define AL_ASSERT				assert

#ifndef EOF
	#define EOF    (-1)
#endif

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

