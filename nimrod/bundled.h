// File generated, see gen_nimrod_c in Makefile
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

#ifndef AL_INLINE
	#define AL_INLINE(type, name, args, code)    static type name args code
#endif

typedef struct PACKFILE_VTABLE_t PACKFILE_VTABLE;
typedef struct PACKFILE_t PACKFILE;
typedef struct LZSS_PACK_DATA_t LZSS_PACK_DATA;
typedef struct LZSS_UNPACK_DATA_t LZSS_UNPACK_DATA;


#endif // ALLEGRO_SIMULATED_BASE_H

// vim:tabstop=4 shiftwidth=4
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
 *      File I/O.
 *
 *      By Shawn Hargreaves.
 *
 *      See Allegro's readme.txt for copyright information.
 */


#ifndef ALLEGRO_FILE_H
#define ALLEGRO_FILE_H


#ifdef __cplusplus
	extern "C" {
#endif


#define F_READ          "r"
#define F_WRITE         "w"
#define F_READ_PACKED   "rp"
#define F_WRITE_PACKED  "wp"
#define F_WRITE_NOPACK  "w!"

/// 4K buffer for caching data
#define F_BUF_SIZE      4096
/// magic number for packed files
#define F_PACK_MAGIC    0x736C6821L
/// magic number for autodetect
#define F_NOPACK_MAGIC  0x736C682EL
/// magic number for appended data
#define F_EXE_MAGIC     0x736C682BL



struct _al_normal_packfile_details
{
	int hndl;                           ///< DOS file handle
	int flags;                          ///< PACKFILE_FLAG_* constants
	unsigned char *buf_pos;             ///< position in buffer
	int buf_size;                       ///< number of bytes in the buffer
	long todo;                          ///< number of bytes still on the disk
	PACKFILE *parent;		            ///< nested, parent file
	LZSS_PACK_DATA *pack_data;			///< for LZSS compression
	LZSS_UNPACK_DATA *unpack_data; 		///< for LZSS decompression
	char *filename;                     ///< name of the file
	char *passdata;                     ///< encryption key data
	char *passpos;                      ///< current key position
	unsigned char buf[F_BUF_SIZE];      ///< the actual data buffer
};


/// Our very own FILE structure...
struct PACKFILE_t
{
	const PACKFILE_VTABLE *vtable;
	void *userdata;
	int is_normal_packfile;

	/* The following is only to be used for the "normal" PACKFILE vtable,
	 * i.e. what is implemented by Allegro itself. If is_normal_packfile is
	 * false then the following is not even allocated. This must be the last
	 * member in the structure.
	 */
	struct _al_normal_packfile_details normal;
};


struct PACKFILE_VTABLE_t
{
	AL_METHOD(int, pf_fclose, (void *userdata));
	AL_METHOD(int, pf_getc, (void *userdata));
	AL_METHOD(int, pf_ungetc, (int c, void *userdata));
	AL_METHOD(long, pf_fread, (void *p, long n, void *userdata));
	AL_METHOD(int, pf_putc, (int c, void *userdata));
	AL_METHOD(long, pf_fwrite, (const void *p, long n, void *userdata));
	AL_METHOD(int, pf_fseek, (void *userdata, int offset));
	AL_METHOD(int, pf_feof, (void *userdata));
	AL_METHOD(int, pf_ferror, (void *userdata));
};


void packfile_password(const char *password);
PACKFILE *pack_fopen(const char *filename, const char *mode);
PACKFILE *pack_fopen_vtable(const PACKFILE_VTABLE *vtable, void *userdata);
int pack_fclose(PACKFILE *f);
int pack_fseek(PACKFILE *f, int offset);
int pack_skip_chunks(PACKFILE *f, unsigned int num_chunks);
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack);
PACKFILE *pack_fclose_chunk(PACKFILE *f);
int pack_getc(PACKFILE *f);
int pack_putc(int c, PACKFILE *f);
int pack_feof(PACKFILE *f);
int pack_ferror(PACKFILE *f);
int pack_igetw(PACKFILE *f);
long pack_igetl(PACKFILE *f);
int pack_iputw(int w, PACKFILE *f);
long pack_iputl(long l, PACKFILE *f);
int pack_mgetw(PACKFILE *f);
long pack_mgetl(PACKFILE *f);
int pack_mputw(int w, PACKFILE *f);
long pack_mputl(long l, PACKFILE *f);
long pack_fread(void *p, long n, PACKFILE *f);
long pack_fwrite(const void *p, long n, PACKFILE *f);
int pack_ungetc(int c, PACKFILE *f);



#ifdef __cplusplus
	}
#endif

#endif          /* ifndef ALLEGRO_FILE_H */

// vim:tabstop=4 shiftwidth=4
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
 *      See Allegro's readme.txt for copyright information.
 */


#ifndef ALLEGRO_LZSS_H
#define ALLEGRO_LZSS_H


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

// vim:tabstop=4 shiftwidth=4
