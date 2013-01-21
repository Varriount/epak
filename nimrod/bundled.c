// File generated, see gen_nimrod_c in Makefile

#ifndef DEBUG
#define NDEBUG
#define NS_BLOCK_ASSERTIONS
#define RELEASE
#endif

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "bundled.h"

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
 *      _pack_fdopen() and related modifications by Annie Testes.
 *
 *      Evert Glebbeek added the support for relative filenames:
 *      make_absolute_filename(), make_relative_filename() and
 *      is_relative_filename().
 *
 *      Peter Wang added support for packfile vtables.
 *
 *      See Allegro's readme.txt for copyright information.
 *
 *      Porting and customization for iPhone, Grzegorz Adam Hankiewicz,
 *      Electric Hands Software.
 */



/* some OSes have no concept of "group" and "other" */
#ifndef S_IRGRP
	#define S_IRGRP	0
	#define S_IWGRP	0
#endif
#ifndef S_IROTH
	#define S_IROTH	0
	#define S_IWOTH	0
#endif

#define OPEN_PERMS	(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)


static char the_password[256] = "";

static int _packfile_filesize = 0;
static int _packfile_datasize = 0;

static int _packfile_type = 0;

static PACKFILE_VTABLE normal_vtable;




/** _al_sane_strncpy:
 *  strncpy() substitution which properly null terminates a string.
 *  By Henrik Stokseth.
 */
static char *_al_sane_strncpy(char *dest, const char *src, size_t n)
{
	if (n <= 0)
		return dest;
	dest[0] = '\0';
	strncat(dest, src, n - 1);

	return dest;
}

/// Checks a path, if it is a directory returns non zero.
static int _exists_dir(const char *path)
{
	struct stat buf;
	if (!stat(path, &buf))
		return (buf.st_mode & S_IFDIR);
	else
		return 0;
}


/***************************************************
 ******************** Packfiles ********************
 ***************************************************/


/** Sets the global encryption password to be used for all read/write
 * operations on files opened in future using Allegro's packfile
 * functions (whether they are compressed or not).
 *
 * Files written with an encryption
 * password cannot be read unless the same password is selected, so
 * be careful: if you forget the key, nobody can make your data come
 * back again! Pass NULL or an empty string to return to the normal,
 * non-encrypted mode. If you are using this function to prevent
 * people getting access to your datafiles, be careful not to store
 * an obvious copy of the password in your executable: if there are
 * any strings like "I'm the password for the datafile", it would be
 * fairly easy to get access to your data :-)
 *
 * Note #1: when writing a packfile, you can change the password
 * to whatever you want after opening the file, without affecting the
 * write operation. On the contrary, when writing a sub-chunk of a
 * packfile, you must make sure that the password that was active at
 * the time the sub-chunk was opened is still active before closing
 * the sub-chunk. This is guaranteed to be true if you didn't call
 * the packfile_password() routine in the meantime. Read operations,
 * either on packfiles or sub-chunks, have no such restriction.
 *
 * Note #2: as explained above, the password is used for all
 * read/write operations on files, including for several functions
 * of the library that operate on files without explicitly using
 * packfiles. The unencrypted mode is mandatory
 * in order for those functions to work. Therefore remember to call
 * packfile_password(NULL) before using them if you previously changed
 * the password. As a rule of thumb, always call packfile_password(NULL)
 * when you are done with operations on packfiles. The only exception
 * to this is custom packfiles created with pack_fopen_vtable().
 */
void packfile_password(const char *password)
{
	int i = 0;
	int c;

	if (password) {
		while ((c = (*password++)) != 0) {
			the_password[i++] = c;
			if (i >= (int)sizeof(the_password)-1)
				break;
		}
	}

	the_password[i] = 0;
}



/* encrypt_id:
 *  Helper for encrypting magic numbers, using the current password.
 */
static int32_t encrypt_id(long x, int new_format)
{
	int32_t mask = 0;
	int i, pos;

	if (the_password[0]) {
		for (i=0; the_password[i]; i++)
			mask ^= ((int32_t)the_password[i] << ((i&3) * 8));

		for (i=0, pos=0; i<4; i++) {
			mask ^= (int32_t)the_password[pos++] << (24-i*8);
			if (!the_password[pos])
				pos = 0;
		}

		if (new_format)
			mask ^= 42;
	}

	return x ^ mask;
}



/* clone_password:
 *  Sets up a local password string for use by this packfile.
 */
static int clone_password(PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->is_normal_packfile);

	if (the_password[0]) {
		if ((f->normal.passdata = _AL_MALLOC_ATOMIC(strlen(the_password)+1)) == NULL) {
			errno = ENOMEM;
			return FALSE;
		}
		_al_sane_strncpy(f->normal.passdata, the_password, strlen(the_password)+1);
		f->normal.passpos = f->normal.passdata;
	}
	else {
		f->normal.passpos = NULL;
		f->normal.passdata = NULL;
	}

	return TRUE;
}



/* create_packfile:
 *  Helper function for creating a PACKFILE structure.
 */
static PACKFILE *create_packfile(int is_normal_packfile)
{
	PACKFILE *f;

	if (is_normal_packfile)
		f = _AL_MALLOC(sizeof(PACKFILE));
	else
		f = _AL_MALLOC(sizeof(PACKFILE) - sizeof(struct _al_normal_packfile_details));

	if (f == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	if (!is_normal_packfile) {
		f->vtable = NULL;
		f->userdata = NULL;
		f->is_normal_packfile = FALSE;
	}
	else {
		f->vtable = &normal_vtable;
		f->userdata = f;
		f->is_normal_packfile = TRUE;

		f->normal.buf_pos = f->normal.buf;
		f->normal.flags = 0;
		f->normal.buf_size = 0;
		f->normal.filename = NULL;
		f->normal.passdata = NULL;
		f->normal.passpos = NULL;
		f->normal.parent = NULL;
		f->normal.pack_data = NULL;
		f->normal.unpack_data = NULL;
		f->normal.todo = 0;
	}

	return f;
}



/* free_packfile:
 *  Helper function for freeing the PACKFILE struct.
 */
static void free_packfile(PACKFILE *f)
{
	if (f) {
		/* These are no longer the responsibility of this function, but
		 * these assertions help catch instances of old code which still
		 * rely on the old behaviour.
		 */
		if (f->is_normal_packfile) {
			AL_ASSERT(!f->normal.pack_data);
			AL_ASSERT(!f->normal.unpack_data);
			AL_ASSERT(!f->normal.passdata);
			AL_ASSERT(!f->normal.passpos);
		}

		_AL_FREE(f);
	}
}



/**
 *  Converts the given file descriptor into a PACKFILE. The mode can have
 *  the same values as for pack_fopen() and must be compatible with the
 *  mode of the file descriptor. Unlike the libc fdopen(), pack_fdopen()
 *  is unable to convert an already partially read or written file (i.e.
 *  the file offset must be 0).
 *
 *  \return On success, it returns a pointer to a file structure, and on error
 *  it returns NULL and stores an error code in errno. An attempt to read
 *  a normal file in packed mode will cause errno to be set to EDOM.
 */
static PACKFILE *_pack_fdopen(int fd, AL_CONST char *mode)
{
	PACKFILE *f, *f2;
	long header = FALSE;
	int c;

	if ((f = create_packfile(TRUE)) == NULL)
		return NULL;

	AL_ASSERT(f->is_normal_packfile);

	while ((c = *(mode++)) != 0) {
		switch (c) {
			case 'r': case 'R': f->normal.flags &= ~PACKFILE_FLAG_WRITE; break;
			case 'w': case 'W': f->normal.flags |= PACKFILE_FLAG_WRITE; break;
			case 'p': case 'P': f->normal.flags |= PACKFILE_FLAG_PACK; break;
			case '!': f->normal.flags &= ~PACKFILE_FLAG_PACK; header = TRUE; break;
		}
	}

	if (f->normal.flags & PACKFILE_FLAG_WRITE) {
		if (f->normal.flags & PACKFILE_FLAG_PACK) {
			/* write a packed file */
			f->normal.pack_data = create_lzss_pack_data();
			AL_ASSERT(!f->normal.unpack_data);

			if (!f->normal.pack_data) {
				free_packfile(f);
				return NULL;
			}

			if ((f->normal.parent = _pack_fdopen(fd, F_WRITE)) == NULL) {
				free_lzss_pack_data(f->normal.pack_data);
				f->normal.pack_data = NULL;
				free_packfile(f);
				return NULL;
			}

			pack_mputl(encrypt_id(F_PACK_MAGIC, TRUE), f->normal.parent);

			f->normal.todo = 4;
		}
		else {
			/* write a 'real' file */
			if (!clone_password(f)) {
				free_packfile(f);
				return NULL;
			}

			f->normal.hndl = fd;
			f->normal.todo = 0;

			errno = 0;

			if (header)
				pack_mputl(encrypt_id(F_NOPACK_MAGIC, TRUE), f);
		}
	}
	else {
		if (f->normal.flags & PACKFILE_FLAG_PACK) {
			/* read a packed file */
			f->normal.unpack_data = create_lzss_unpack_data();
			AL_ASSERT(!f->normal.pack_data);

			if (!f->normal.unpack_data) {
				free_packfile(f);
				return NULL;
			}

			if ((f->normal.parent = _pack_fdopen(fd, F_READ)) == NULL) {
				free_lzss_unpack_data(f->normal.unpack_data);
				f->normal.unpack_data = NULL;
				free_packfile(f);
				return NULL;
			}

			header = pack_mgetl(f->normal.parent);

			if ((f->normal.parent->normal.passpos) &&
				 ((header == encrypt_id(F_PACK_MAGIC, FALSE)) ||
				  (header == encrypt_id(F_NOPACK_MAGIC, FALSE))))
			{
				/* duplicate the file descriptor */
				int fd2 = dup(fd);

				if (fd2<0) {
					pack_fclose(f->normal.parent);
					free_packfile(f);
					return NULL;
				}

				/* close the parent file (logically, not physically) */
				pack_fclose(f->normal.parent);

				/* backward compatibility mode */
				if (!clone_password(f)) {
					free_packfile(f);
					return NULL;
				}

				f->normal.flags |= PACKFILE_FLAG_OLD_CRYPT;

				/* re-open the parent file */
				lseek(fd2, 0, SEEK_SET);

				if ((f->normal.parent = _pack_fdopen(fd2, F_READ)) == NULL) {
					free_packfile(f);
					return NULL;
				}

				f->normal.parent->normal.flags |= PACKFILE_FLAG_OLD_CRYPT;

				pack_mgetl(f->normal.parent);

				if (header == encrypt_id(F_PACK_MAGIC, FALSE))
					header = encrypt_id(F_PACK_MAGIC, TRUE);
				else
					header = encrypt_id(F_NOPACK_MAGIC, TRUE);
			}

			if (header == encrypt_id(F_PACK_MAGIC, TRUE)) {
				f->normal.todo = LONG_MAX;
			}
			else if (header == encrypt_id(F_NOPACK_MAGIC, TRUE)) {
				f2 = f->normal.parent;
				free_lzss_unpack_data(f->normal.unpack_data);
				f->normal.unpack_data = NULL;
				free_packfile(f);
				return f2;
			}
			else {
				pack_fclose(f->normal.parent);
				free_lzss_unpack_data(f->normal.unpack_data);
				f->normal.unpack_data = NULL;
				free_packfile(f);
				errno = EDOM;
				return NULL;
			}
		}
		else {
			/* read a 'real' file */
			f->normal.todo = lseek(fd, 0, SEEK_END);	/* size of the file */
			if (f->normal.todo < 0) {
				free_packfile(f);
				return NULL;
			}

			lseek(fd, 0, SEEK_SET);

			if (!clone_password(f)) {
				free_packfile(f);
				return NULL;
			}

			f->normal.hndl = fd;
		}
	}

	return f;
}


/** Opens a file according to mode, which may contain any of the flags:
 *
 * - r: open file for reading.
 * - w: open file for writing, overwriting any existing data.
 * - p: open file in packed mode. Data will be compressed as it is
 *      written to the file, and automatically uncompressed during read
 *      operations. Files created in this mode will produce garbage if
 *      they are read without this flag being set.
 * - !: open file for writing in normal, unpacked mode, but add the
 *      value ::F_NOPACK_MAGIC to the start of the file, so that it can later
 *      be opened in packed mode and Allegro will automatically detect
 *      that the data does not need to be decompressed.
 *
 * Instead of these flags, one of the constants ::F_READ, ::F_WRITE,
 * ::F_READ_PACKED, ::F_WRITE_PACKED or ::F_WRITE_NOPACK may be used as the
 * mode parameter.
 *
 * Example:
 * \code
 *	PACKFILE *input_file;
 *
 *	input_file = pack_fopen("scores.dat", "rp");
 *	if (!input_file)
 *		abort_on_error("Couldn't read `scores.dat'!");
 * \endcode
 *
 * \return On success, pack_fopen() returns a pointer to a
 * PACKFILE structure, and on error it returns NULL and stores an
 * error code in errno. An attempt to read a normal file in packed
 * mode will cause errno to be set to EDOM.
 */
PACKFILE *pack_fopen(const char *filename, const char *mode)
{
	int fd;
	AL_ASSERT(filename);

	_packfile_type = 0;

#ifndef ALLEGRO_MPW
	if (strpbrk(mode, "wW"))  /* write mode? */
		fd = _al_open(filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, OPEN_PERMS);
	else
		fd = _al_open(filename, O_RDONLY | O_BINARY, OPEN_PERMS);
#else
	if (strpbrk(mode, "wW"))  /* write mode? */
		fd = _al_open(filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC);
	else
		fd = _al_open(filename, O_RDONLY | O_BINARY);
#endif

	if (fd < 0) {
		return NULL;
	}

	return _pack_fdopen(fd, mode);
}



/** Creates a new packfile structure that uses the functions specified in
 *  the vtable instead of the standard functions.	On success, it returns a
 *  pointer to a file structure, and on error it returns NULL and
 *  stores an error code in errno.
 *
 *  The vtable and userdata must remain available for the lifetime of the
 *  created packfile.
 *
 *  Opening chunks using pack_fopen_chunk() on top of the returned packfile
 *  is not possible at this time.
 *
 *  packfile_password() does not have any effect on packfiles opened
 *  with pack_fopen_vtable().
 */
PACKFILE *pack_fopen_vtable(const PACKFILE_VTABLE *vtable, void *userdata)
{
	PACKFILE *f;
	AL_ASSERT(vtable);
	AL_ASSERT(vtable->pf_fclose);
	AL_ASSERT(vtable->pf_getc);
	AL_ASSERT(vtable->pf_ungetc);
	AL_ASSERT(vtable->pf_fread);
	AL_ASSERT(vtable->pf_putc);
	AL_ASSERT(vtable->pf_fwrite);
	AL_ASSERT(vtable->pf_fseek);
	AL_ASSERT(vtable->pf_feof);
	AL_ASSERT(vtable->pf_ferror);

	if ((f = create_packfile(FALSE)) == NULL)
		return NULL;

	f->vtable = vtable;
	f->userdata = userdata;
	AL_ASSERT(!f->is_normal_packfile);

	return f;
}


/** Closes a stream previously opened with pack_fopen() or
 * pack_fopen_vtable(). After you have closed the stream, performing
 * operations on it will yield errors in your application (e.g. crash
 * it) or even block your OS.
 *
 * \return Returns zero on success. On error, returns an error code
 * which is also stored in `errno'. This function can fail only when
 * writing to files: if the file was opened in read mode, it will
 * always succeed.
 */
int pack_fclose(PACKFILE *f)
{
	int ret;

	if (!f)
		return 0;

	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_fclose);

	ret = f->vtable->pf_fclose(f->userdata);
	free_packfile(f);

	return ret;
}


/** Opens a sub-chunk of a file. A chunk provides a logical view of
 * part of a file, which can be compressed as an individual entity
 * and will automatically insert and check length counts to prevent
 * reading past the end of the chunk. The PACKFILE parameter is a
 * previously opened file, and `pack' is a boolean parameter which
 * will turn compression on for the sub-chunk if it is non-zero.
 *
 * Example:
 * \code
 *	PACKFILE *output = pack_fopen("out.raw", "w!");
 *	...
 *	// Create a sub-chunk with compression.
 *	output = pack_fopen(chunk(output, 1);
 *	if (!output)
 *	   abort_on_error("Error saving data!");
 *	// Write some data to the sub-chunk.
 *	...
 *	// Close the sub-chunk, recovering parent file.
 *	output = pack_fclose_chunk(output);
 * \endcode
 *
 * The data written to the chunk will be prefixed with two length
 * counts (32-bit, a.k.a. big-endian). For uncompressed chunks these
 * will both be set to the size of the data in the chunk. For compressed
 * chunks (created by setting the `pack' flag), the first length will
 * be the raw size of the chunk, and the second will be the negative
 * size of the uncompressed data.
 *
 * To read the chunk, use the following code:
 * \code
 *	PACKFILE *input = pack_fopen("out.raw", "rp");
 *	...
 *	input = pack_fopen_chunk(input, 1);
 *	// Read data from the sub-chunk and close it.
 *	...
 *	input = pack_fclose_chunk(input);
 * \endcode
 *
 * This sequence will read the length counts created when the chunk
 * was written, and automatically decompress the contents of the chunk
 * if it was compressed. The length will also be used to prevent
 * reading past the end of the chunk (Allegro will return EOF if you
 * attempt this), and to automatically skip past any unread chunk
 * data when you call pack_fclose_chunk(). This means that you can skip
 * a whole chunk by calling pack_fopen_chunk() and pack_fclose_chunk() in
 * sucession, though it is faster to call pack_skip_chunks().
 *
 * Chunks can be nested inside each other by making repeated calls
 * to pack_fopen_chunk(). When writing a file, the compression status
 * is inherited from the parent file, so you only need to set the
 * pack flag if the parent is not compressed but you want to pack the
 * chunk data. If the parent file is already open in packed mode,
 * setting the pack flag will result in data being compressed twice:
 * once as it is written to the chunk, and again as the chunk passes
 * it on to the parent file.
 *
 * \return Returns a pointer to the sub-chunked PACKFILE, or NULL
 * if there was some error (eg. you are using a custom PACKFILE
 * vtable).
 */
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack)
{
	PACKFILE *chunk;
	char *name;
	AL_ASSERT(f);

	/* unsupported */
	if (!f->is_normal_packfile) {
		errno = EINVAL;
		return NULL;
	}

	if (f->normal.flags & PACKFILE_FLAG_WRITE) {

		/* write a sub-chunk */
		int tmp_fd = -1;
		char *tmp_dir = NULL;
		char *tmp_name = NULL;

		/* Get the path of the temporary directory */

		/* Try various possible locations to store the temporary file */
		if (getenv("TEMP")) {
			tmp_dir = strdup(getenv("TEMP"));
		}
		else if (getenv("TMP")) {
			tmp_dir = strdup(getenv("TMP"));
		}
		else if (_exists_dir("/tmp")) {
			tmp_dir = strdup("/tmp");
		}
		else if (getenv("HOME")) {
			tmp_dir = strdup(getenv("HOME"));
		}
		else {
			/* Give up - try current directory */
			tmp_dir = strdup(".");
		}

		/* the file is open in read/write mode, even if the pack file
		 * seems to be in write only mode
		 */
		tmp_name = _AL_MALLOC_ATOMIC(strlen(tmp_dir) + 16);
		sprintf(tmp_name, "%s/XXXXXX", tmp_dir);
		tmp_fd = mkstemp(tmp_name);

		if (tmp_fd < 0) {
			_AL_FREE(tmp_dir);
			_AL_FREE(tmp_name);

			return NULL;
		}

		name = tmp_name;
		chunk = _pack_fdopen(tmp_fd, (pack ? F_WRITE_PACKED : F_WRITE_NOPACK));

		if (chunk) {
			chunk->normal.filename = strdup(name);

			if (pack)
				chunk->normal.parent->normal.parent = f;
			else
				chunk->normal.parent = f;

			chunk->normal.flags |= PACKFILE_FLAG_CHUNK;
		}

		_AL_FREE(tmp_dir);
		_AL_FREE(tmp_name);
	}
	else {
		/* read a sub-chunk */
		_packfile_filesize = pack_mgetl(f);
		_packfile_datasize = pack_mgetl(f);

		if ((chunk = create_packfile(TRUE)) == NULL)
			return NULL;

		chunk->normal.flags = PACKFILE_FLAG_CHUNK;
		chunk->normal.parent = f;

		if (f->normal.flags & PACKFILE_FLAG_OLD_CRYPT) {
			/* backward compatibility mode */
			if (f->normal.passdata) {
				if ((chunk->normal.passdata = _AL_MALLOC_ATOMIC(strlen(f->normal.passdata)+1)) == NULL) {
					errno = ENOMEM;
					_AL_FREE(chunk);
					return NULL;
				}
				_al_sane_strncpy(chunk->normal.passdata, f->normal.passdata, strlen(f->normal.passdata)+1);
				chunk->normal.passpos = chunk->normal.passdata + (long)f->normal.passpos - (long)f->normal.passdata;
				f->normal.passpos = f->normal.passdata;
			}
			chunk->normal.flags |= PACKFILE_FLAG_OLD_CRYPT;
		}

		if (_packfile_datasize < 0) {
			/* read a packed chunk */
			chunk->normal.unpack_data = create_lzss_unpack_data();
			AL_ASSERT(!chunk->normal.pack_data);

			if (!chunk->normal.unpack_data) {
				free_packfile(chunk);
				return NULL;
			}

			_packfile_datasize = -_packfile_datasize;
			chunk->normal.todo = _packfile_datasize;
			chunk->normal.flags |= PACKFILE_FLAG_PACK;
		}
		else {
			/* read an uncompressed chunk */
			chunk->normal.todo = _packfile_datasize;
		}
	}

	return chunk;
}


/** Closes a sub-chunk of a file, previously obtained by calling
 * pack_fopen_chunk().
 *
 * \return Returns a pointer to the parent of the sub-chunk you
 * just closed. Returns NULL if there was some error (eg. you tried
 * to close a PACKFILE which wasn't sub-chunked).
 */
PACKFILE *pack_fclose_chunk(PACKFILE *f)
{
	PACKFILE *parent;
	PACKFILE *tmp;
	char *name;
	int header, c;
	AL_ASSERT(f);

	/* unsupported */
	if (!f->is_normal_packfile) {
		errno = EINVAL;
		return NULL;
	}

	parent = f->normal.parent;
	AL_ASSERT(parent);
	name = f->normal.filename;

	if (f->normal.flags & PACKFILE_FLAG_WRITE) {
		/* finish writing a chunk */
		int hndl;

		/* duplicate the file descriptor to create a readable pack file,
		 * the file descriptor must have been opened in read/write mode
		 */
		if (f->normal.flags & PACKFILE_FLAG_PACK)
			hndl = dup(f->normal.parent->normal.hndl);
		else
			hndl = dup(f->normal.hndl);

		if (hndl<0) {
			return NULL;
		}

		_packfile_datasize = f->normal.todo + f->normal.buf_size - 4;

		if (f->normal.flags & PACKFILE_FLAG_PACK) {
			parent = parent->normal.parent;
			f->normal.parent->normal.parent = NULL;
		}
		else
			f->normal.parent = NULL;

		/* close the writeable temp file, it isn't physically closed
		 * because the descriptor has been duplicated
		 */
		f->normal.flags &= ~PACKFILE_FLAG_CHUNK;
		pack_fclose(f);

		lseek(hndl, 0, SEEK_SET);

		/* create a readable pack file */
		tmp = _pack_fdopen(hndl, F_READ);
		if (!tmp)
			return NULL;

		_packfile_filesize = tmp->normal.todo - 4;

		header = pack_mgetl(tmp);

		pack_mputl(_packfile_filesize, parent);

		if (header == encrypt_id(F_PACK_MAGIC, TRUE))
			pack_mputl(-_packfile_datasize, parent);
		else
			pack_mputl(_packfile_datasize, parent);

		while ((c = pack_getc(tmp)) != EOF)
			pack_putc(c, parent);

		pack_fclose(tmp);

		unlink(name);
		_AL_FREE(name);
	}
	else {
		/* finish reading a chunk */
		while (f->normal.todo > 0)
			pack_getc(f);

		if (f->normal.unpack_data) {
			free_lzss_unpack_data(f->normal.unpack_data);
			f->normal.unpack_data = NULL;
		}

		if ((f->normal.passpos) && (f->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
			parent->normal.passpos = parent->normal.passdata + (long)f->normal.passpos - (long)f->normal.passdata;

		free_packfile(f);
	}

	return parent;
}



/** Seeks inside a stream.
 * Unlike the standard fseek() function, this only supports forward movements
 * relative to the current position and in read-only streams, so don't
 * use negative offsets. Note that seeking is very slow when reading
 * compressed files, and so should be avoided unless you are sure
 * that the file is not compressed.
 *
 * Example:
 * \code
 *	input_file = pack_fopen("data.bin", "r");
 *	if (!input_file)
 *		abort_on_error("Couldn't open binary data!");
 *	// Skip some useless header before reading data.
 *	pack_fseek(input_file, 32);
 * \endcode
 *
 * \return Returns zero on success or a negative number on error,
 * storing the error code in `errno'.
 */
int pack_fseek(PACKFILE *f, int offset)
{
	AL_ASSERT(f);
	AL_ASSERT(offset >= 0);

	return f->vtable->pf_fseek(f->userdata, offset);
}


/** Skips a number of subchunks inside the file.
 * This is faster than opening and closing a subchunk as you find it on disk.
 * The function reads the hidden chunk size data and uses that to call
 * pack_fseek instead. Pass how many chunks you want to skip, usually one.
 *
 * \return Returns non zero on error, storing the code in errno.
 */
int pack_skip_chunks(PACKFILE *f, unsigned int num_chunks)
{
	if (!num_chunks)
		return 0;

	const int filesize = pack_mgetl(f);
#ifdef DEBUG
	const int datasize = pack_mgetl(f);
#else
	pack_mgetl(f);
#endif
	AL_ASSERT(filesize >= 0 && (datasize || !datasize));

	if (pack_fseek(f, filesize))
		return 1;

	if (num_chunks)
		pack_skip_chunks(f, num_chunks - 1);

	return 0;
}

/**
 * Returns the next character from the stream f, or EOF if the end of the
 * file has been reached.
 */
int pack_getc(PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_getc);

	return f->vtable->pf_getc(f->userdata);
}



/**
 * Puts a character in the stream f.
 */
int pack_putc(int c, PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_putc);

	return f->vtable->pf_putc(c, f->userdata);
}



/** Returns nonzero as soon as you reach the end of the file.
 * It does not wait for you to attempt to read beyond the end of
 * the file, contrary to the ISO C feof() function. The only way to
 * know whether you have read beyond the end of the file is to check
 * the return value of the read operation you use (and be wary of
 * pack_*getl() as EOF is also a valid return value with these functions).
 *
 * \return Returns non-zero if you are at the end of the file, zero otherwise.
 */
int pack_feof(PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_feof);

	return f->vtable->pf_feof(f->userdata);
}



/**Tells if an error occurred during an operation on the stream.
 * Since EOF is used to report errors by some functions, it's often
 * better to use the pack_feof() function to check explicitly for end
 * of file and pack_ferror() to check for errors. Both functions check
 * indicators that are part of the internal state of the stream to
 * detect correctly the different situations.
 *
 * \return Returns nonzero if the error indicator for the stream
 * is set, meaning that an error has occurred during a previous
 * operation on the stream.
 */
int pack_ferror(PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_ferror);

	return f->vtable->pf_ferror(f->userdata);
}



/**
 * Reads a 16 bit word from a file, using intel byte ordering.
 */
int pack_igetw(PACKFILE *f)
{
	int b1, b2;
	AL_ASSERT(f);

	if ((b1 = pack_getc(f)) != EOF)
		if ((b2 = pack_getc(f)) != EOF)
			return ((b2 << 8) | b1);

	return EOF;
}



/**
 *  Reads a 32 bit long from a file, using intel byte ordering.
 */
long pack_igetl(PACKFILE *f)
{
	int b1, b2, b3, b4;
	AL_ASSERT(f);

	if ((b1 = pack_getc(f)) != EOF)
		if ((b2 = pack_getc(f)) != EOF)
			if ((b3 = pack_getc(f)) != EOF)
				if ((b4 = pack_getc(f)) != EOF)
					return (((long)b4 << 24) | ((long)b3 << 16) |
							  ((long)b2 << 8) | (long)b1);

	return EOF;
}



/**
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, PACKFILE *f)
{
	int b1, b2;
	AL_ASSERT(f);

	b1 = (w & 0xFF00) >> 8;
	b2 = w & 0x00FF;

	if (pack_putc(b2,f)==b2)
		if (pack_putc(b1,f)==b1)
			return w;

	return EOF;
}



/**
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, PACKFILE *f)
{
	int b1, b2, b3, b4;
	AL_ASSERT(f);

	b1 = (int)((l & 0xFF000000L) >> 24);
	b2 = (int)((l & 0x00FF0000L) >> 16);
	b3 = (int)((l & 0x0000FF00L) >> 8);
	b4 = (int)l & 0x00FF;

	if (pack_putc(b4,f)==b4)
		if (pack_putc(b3,f)==b3)
			if (pack_putc(b2,f)==b2)
				if (pack_putc(b1,f)==b1)
					return l;

	return EOF;
}



/**
 *  Reads a 16 bit int from a file, using motorola byte-ordering.
 */
int pack_mgetw(PACKFILE *f)
{
	int b1, b2;
	AL_ASSERT(f);

	if ((b1 = pack_getc(f)) != EOF)
		if ((b2 = pack_getc(f)) != EOF)
			return ((b1 << 8) | b2);

	return EOF;
}



/**
 * Reads a 32 bit long from a file, using motorola byte-ordering.
 */
long pack_mgetl(PACKFILE *f)
{
	int b1, b2, b3, b4;
	AL_ASSERT(f);

	if ((b1 = pack_getc(f)) != EOF)
		if ((b2 = pack_getc(f)) != EOF)
			if ((b3 = pack_getc(f)) != EOF)
				if ((b4 = pack_getc(f)) != EOF)
					return (((long)b1 << 24) | ((long)b2 << 16) |
							  ((long)b3 << 8) | (long)b4);

	return EOF;
}



/**
 *  Writes a 16 bit int to a file, using motorola byte-ordering.
 */
int pack_mputw(int w, PACKFILE *f)
{
	int b1, b2;
	AL_ASSERT(f);

	b1 = (w & 0xFF00) >> 8;
	b2 = w & 0x00FF;

	if (pack_putc(b1,f)==b1)
		if (pack_putc(b2,f)==b2)
			return w;

	return EOF;
}



/**
 *  Writes a 32 bit long to a file, using motorola byte-ordering.
 */
long pack_mputl(long l, PACKFILE *f)
{
	int b1, b2, b3, b4;
	AL_ASSERT(f);

	b1 = (int)((l & 0xFF000000L) >> 24);
	b2 = (int)((l & 0x00FF0000L) >> 16);
	b3 = (int)((l & 0x0000FF00L) >> 8);
	b4 = (int)l & 0x00FF;

	if (pack_putc(b1,f)==b1)
		if (pack_putc(b2,f)==b2)
			if (pack_putc(b3,f)==b3)
				if (pack_putc(b4,f)==b4)
					return l;

	return EOF;
}



/** Reads n bytes from f and stores them at memory location p.
 * Example:
 * \code
 *	unsigned char buf[256];
 *	...
 *	if (256 != pack_fread(buf, 256, input_file))
 *		abort_on_error("Truncated input file!");
 * \endcode
 *
 * \return Returns the number of bytes read, which will be less
 * than `n' if EOF is reached or an error occurs. Error codes are
 * stored in errno.
 */
long pack_fread(void *p, long n, PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_fread);
	AL_ASSERT(p);
	AL_ASSERT(n >= 0);

	return f->vtable->pf_fread(p, n, f->userdata);
}



/**
 *  Writes n bytes to the file f from memory location p.
 *
 *  \return Returns the number
 *  of items written, which will be less than n if an error occurs. Error
 *  codes are stored in errno.
 */
long pack_fwrite(const void *p, long n, PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_fwrite);
	AL_ASSERT(p);
	AL_ASSERT(n >= 0);

	return f->vtable->pf_fwrite(p, n, f->userdata);
}



/** Moves one single character back to the input buffer.
 * Like with ungetc from libc, only a single push back is guaranteed.
 *
 * Note: pack_fgets internally uses pack_ungetc(), so never use
 * pack_ungetc() directly after using pack_fgets() on a PACKFILE.
 *
 * \return Returns c on success, or EOF on error.
 */
int pack_ungetc(int c, PACKFILE *f)
{
	AL_ASSERT(f);
	AL_ASSERT(f->vtable);
	AL_ASSERT(f->vtable->pf_ungetc);

	return f->vtable->pf_ungetc(c, f->userdata);
}




/***************************************************
 ************ "Normal" packfile vtable *************
 ***************************************************

	Ideally this would be the only section which knows about the details
	of "normal" packfiles. However, this ideal is still being violated in
	many places (partly due to the API, and partly because it would be
	quite a lot of work to move the _al_normal_packfile_details field
	into the userdata field of the PACKFILE structure.
*/


static int normal_fclose(void *_f);
static int normal_getc(void *_f);
static int normal_ungetc(int ch, void *_f);
static int normal_putc(int c, void *_f);
static long normal_fread(void *p, long n, void *_f);
static long normal_fwrite(AL_CONST void *p, long n, void *_f);
static int normal_fseek(void *_f, int offset);
static int normal_feof(void *_f);
static int normal_ferror(void *_f);

static int normal_refill_buffer(PACKFILE *f);
static int normal_flush_buffer(PACKFILE *f, int last);



static PACKFILE_VTABLE normal_vtable =
{
	normal_fclose,
	normal_getc,
	normal_ungetc,
	normal_fread,
	normal_putc,
	normal_fwrite,
	normal_fseek,
	normal_feof,
	normal_ferror
};



static int normal_fclose(void *_f)
{
	PACKFILE *f = _f;
	int ret;

	if (f->normal.flags & PACKFILE_FLAG_WRITE) {
		if (f->normal.flags & PACKFILE_FLAG_CHUNK) {
			f = pack_fclose_chunk(_f);
			if (!f)
				return -1;

			return pack_fclose(f);
		}

		normal_flush_buffer(f, TRUE);
	}

	if (f->normal.parent) {
		ret = pack_fclose(f->normal.parent);
	}
	else {
		ret = close(f->normal.hndl);
	}

	if (f->normal.pack_data) {
		free_lzss_pack_data(f->normal.pack_data);
		f->normal.pack_data = NULL;
	}

	if (f->normal.unpack_data) {
		free_lzss_unpack_data(f->normal.unpack_data);
		f->normal.unpack_data = NULL;
	}

	if (f->normal.passdata) {
		_AL_FREE(f->normal.passdata);
		f->normal.passdata = NULL;
		f->normal.passpos = NULL;
	}

	return ret;
}



/* normal_no_more_input:
 *  Return non-zero if the number of bytes remaining in the file (todo) is
 *  less than or equal to zero.
 *
 *  However, there is a special case.	If we are reading from a LZSS
 *  compressed file, the latest call to lzss_read() may have suspended while
 *  writing out a sequence of bytes due to the output buffer being too small.
 *  In that case the `todo' count would be decremented (possibly to zero),
 *  but the output isn't yet completely written out.
 */
static INLINE int normal_no_more_input(PACKFILE *f)
{
	/* see normal_refill_buffer() to see when lzss_read() is called */
	if (f->normal.parent && (f->normal.flags & PACKFILE_FLAG_PACK) &&
		 _al_lzss_incomplete_state(f->normal.unpack_data))
		return 0;

	return (f->normal.todo <= 0);
}



static int normal_getc(void *_f)
{
	PACKFILE *f = _f;

	f->normal.buf_size--;
	if (f->normal.buf_size > 0)
		return *(f->normal.buf_pos++);

	if (f->normal.buf_size == 0) {
		if (normal_no_more_input(f))
			f->normal.flags |= PACKFILE_FLAG_EOF;
		return *(f->normal.buf_pos++);
	}

	return normal_refill_buffer(f);
}



static int normal_ungetc(int c, void *_f)
{
	PACKFILE *f = _f;

	if (f->normal.buf_pos == f->normal.buf) {
		return EOF;
	}
	else {
		*(--f->normal.buf_pos) = (unsigned char)c;
		f->normal.buf_size++;
		f->normal.flags &= ~PACKFILE_FLAG_EOF;
		return (unsigned char)c;
	}
}



static long normal_fread(void *p, long n, void *_f)
{
	PACKFILE *f = _f;
	unsigned char *cp = (unsigned char *)p;
	long i;
	int c;

	for (i=0; i<n; i++) {
		if ((c = normal_getc(f)) == EOF)
			break;

		*(cp++) = c;
	}

	return i;
}



static int normal_putc(int c, void *_f)
{
	PACKFILE *f = _f;

	if (f->normal.buf_size + 1 >= F_BUF_SIZE) {
		if (normal_flush_buffer(f, FALSE))
			return EOF;
	}

	f->normal.buf_size++;
	return (*(f->normal.buf_pos++) = c);
}



static long normal_fwrite(AL_CONST void *p, long n, void *_f)
{
	PACKFILE *f = _f;
	AL_CONST unsigned char *cp = (AL_CONST unsigned char *)p;
	long i;

	for (i=0; i<n; i++) {
		if (normal_putc(*cp++, f) == EOF)
			break;
	}

	return i;
}



static int normal_fseek(void *_f, int offset)
{
	PACKFILE *f = _f;
	int i;

	if (f->normal.flags & PACKFILE_FLAG_WRITE)
		return -1;

	errno = 0;

	/* skip forward through the buffer */
	if (f->normal.buf_size > 0) {
		i = AL_MIN(offset, f->normal.buf_size);
		f->normal.buf_size -= i;
		f->normal.buf_pos += i;
		offset -= i;
		if ((f->normal.buf_size <= 0) && normal_no_more_input(f))
			f->normal.flags |= PACKFILE_FLAG_EOF;
	}

	/* need to seek some more? */
	if (offset > 0) {
		i = AL_MIN(offset, f->normal.todo);

		if ((f->normal.flags & PACKFILE_FLAG_PACK) || (f->normal.passpos)) {
			/* for compressed or encrypted files, we just have to read through the data */
			while (i > 0) {
				pack_getc(f);
				i--;
			}
		}
		else {
			if (f->normal.parent) {
				/* pass the seek request on to the parent file */
				pack_fseek(f->normal.parent, i);
			}
			else {
				/* do a real seek */
				lseek(f->normal.hndl, i, SEEK_CUR);
			}
			f->normal.todo -= i;
			if (normal_no_more_input(f))
				f->normal.flags |= PACKFILE_FLAG_EOF;
		}
	}

	if (errno)
		return -1;
	else
		return 0;
}



static int normal_feof(void *_f)
{
	PACKFILE *f = _f;

	return (f->normal.flags & PACKFILE_FLAG_EOF);
}



static int normal_ferror(void *_f)
{
	PACKFILE *f = _f;

	return (f->normal.flags & PACKFILE_FLAG_ERROR);
}



/* normal_refill_buffer:
 *  Refills the read buffer. The file must have been opened in read mode,
 *  and the buffer must be empty.
 */
static int normal_refill_buffer(PACKFILE *f)
{
	int i, sz, done, offset;

	if (f->normal.flags & PACKFILE_FLAG_EOF)
		return EOF;

	if (normal_no_more_input(f)) {
		f->normal.flags |= PACKFILE_FLAG_EOF;
		return EOF;
	}

	if (f->normal.parent) {
		if (f->normal.flags & PACKFILE_FLAG_PACK) {
			f->normal.buf_size = lzss_read(f->normal.parent,
				f->normal.unpack_data, AL_MIN(F_BUF_SIZE, f->normal.todo),
				f->normal.buf);
		}
		else {
			f->normal.buf_size = pack_fread(f->normal.buf,
				AL_MIN(F_BUF_SIZE, f->normal.todo), f->normal.parent);
		}
		if (f->normal.parent->normal.flags & PACKFILE_FLAG_EOF)
			f->normal.todo = 0;
		if (f->normal.parent->normal.flags & PACKFILE_FLAG_ERROR)
			goto Error;
	}
	else {
		f->normal.buf_size = AL_MIN(F_BUF_SIZE, f->normal.todo);

		offset = lseek(f->normal.hndl, 0, SEEK_CUR);
		done = 0;

		errno = 0;
		sz = read(f->normal.hndl, f->normal.buf, f->normal.buf_size);

		while (sz+done < f->normal.buf_size) {
			if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
				goto Error;

			if (sz > 0)
				done += sz;

			lseek(f->normal.hndl, offset+done, SEEK_SET);
			errno = 0;
			sz = read(f->normal.hndl, f->normal.buf+done, f->normal.buf_size-done);
		}

		if ((f->normal.passpos) && (!(f->normal.flags & PACKFILE_FLAG_OLD_CRYPT))) {
			for (i=0; i<f->normal.buf_size; i++) {
				f->normal.buf[i] ^= *(f->normal.passpos++);
				if (!*f->normal.passpos)
					f->normal.passpos = f->normal.passdata;
			}
		}
	}

	f->normal.todo -= f->normal.buf_size;
	f->normal.buf_pos = f->normal.buf;
	f->normal.buf_size--;
	if (f->normal.buf_size <= 0)
		if (normal_no_more_input(f))
			f->normal.flags |= PACKFILE_FLAG_EOF;

	if (f->normal.buf_size < 0)
		return EOF;
	else
		return *(f->normal.buf_pos++);

Error:
	errno = EFAULT;
	f->normal.flags |= PACKFILE_FLAG_ERROR;
	return EOF;
}



/* normal_flush_buffer:
 *  Flushes a file buffer to the disk. The file must be open in write mode.
 */
static int normal_flush_buffer(PACKFILE *f, int last)
{
	int i, sz, done, offset;

	if (f->normal.buf_size > 0) {
		if (f->normal.flags & PACKFILE_FLAG_PACK) {
			if (lzss_write(f->normal.parent, f->normal.pack_data, f->normal.buf_size, f->normal.buf, last))
				goto Error;
		}
		else {
			if ((f->normal.passpos) && (!(f->normal.flags & PACKFILE_FLAG_OLD_CRYPT))) {
				for (i=0; i<f->normal.buf_size; i++) {
					f->normal.buf[i] ^= *(f->normal.passpos++);
					if (!*f->normal.passpos)
						f->normal.passpos = f->normal.passdata;
				}
			}

			offset = lseek(f->normal.hndl, 0, SEEK_CUR);
			done = 0;

			errno = 0;
			sz = write(f->normal.hndl, f->normal.buf, f->normal.buf_size);

			while (sz+done < f->normal.buf_size) {
				if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
					goto Error;

				if (sz > 0)
					done += sz;

				lseek(f->normal.hndl, offset+done, SEEK_SET);
				errno = 0;
				sz = write(f->normal.hndl, f->normal.buf+done, f->normal.buf_size-done);
			}
		}
		f->normal.todo += f->normal.buf_size;
	}

	f->normal.buf_pos = f->normal.buf;
	f->normal.buf_size = 0;
	return 0;

Error:
	errno = EFAULT;
	f->normal.flags |= PACKFILE_FLAG_ERROR;
	return EOF;
}

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
 *      LZSS compression routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Original code by Haruhiko Okumura.
 *
 *      See Allegro's readme.txt for copyright information.
 */





/*
   This compression algorithm is based on the ideas of Lempel and Ziv,
   with the modifications suggested by Storer and Szymanski. The algorithm
   is based on the use of a ring buffer, which initially contains zeros.
   We read several characters from the file into the buffer, and then
   search the buffer for the longest string that matches the characters
   just read, and output the length and position of the match in the buffer.

   With a buffer size of 4096 bytes, the position can be encoded in 12
   bits. If we represent the match length in four bits, the <position,
   length> pair is two bytes long. If the longest match is no more than
   two characters, then we send just one character without encoding, and
   restart the process with the next letter. We must send one extra bit
   each time to tell the decoder whether we are sending a <position,
   length> pair or an unencoded character, and these flags are stored as
   an eight bit mask every eight items.

   This implementation uses binary trees to speed up the search for the
   longest match.

   Original code by Haruhiko Okumura, 4/6/1989.
   12-2-404 Green Heights, 580 Nagasawa, Yokosuka 239, Japan.

   Modified for use in the Allegro filesystem by Shawn Hargreaves.

   Porting and customization for iPhone, Grzegorz Adam Hankiewicz,
   Electric Hands Software.

   Use, distribute, and modify this code freely.
*/


#define N				4096		/* 4k buffers for LZ compression */
#define F				18			/* upper limit for LZ match length */
#define THRESHOLD		2			/* LZ encode string into pos and length
									   if match size is greater than this */

struct LZSS_PACK_DATA_t					/* stuff for doing LZ compression */
{
	int state;							/* where have we got to in the pack? */
	int i, c, len, r, s;
	int last_match_length, code_buf_ptr;
	unsigned char mask;
	char code_buf[17];
	int match_position;
	int match_length;
	int lson[N+1];					/* left children, */
	int rson[N+257];				/* right children, */
	int dad[N+1];					/* and parents, = binary search trees */
	unsigned char text_buf[N+F-1];	/* ring buffer, with F-1 extra bytes
									   for string comparison */
};


struct LZSS_UNPACK_DATA_t			/* for reading LZ files */
{
	int state;						/* where have we got to? */
	int i, j, k, r, c;
	int flags;
	unsigned char text_buf[N+F-1];	/* ring buffer, with F-1 extra bytes
									   for string comparison */
};



/*** Compression (writing) ***/

/**
 *  Creates a PACK_DATA structure.
 */
LZSS_PACK_DATA *create_lzss_pack_data(void)
{
	LZSS_PACK_DATA *dat;
	int c;

	if ((dat = _AL_MALLOC_ATOMIC(sizeof(LZSS_PACK_DATA))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	for (c=0; c < N - F; c++)
		dat->text_buf[c] = 0;

	dat->state = 0;

	return dat;
}



/**
 *  Frees an LZSS_PACK_DATA structure.
 */
void free_lzss_pack_data(LZSS_PACK_DATA *dat)
{
	AL_ASSERT(dat);

	_AL_FREE(dat);
}



/**
 *  For i = 0 to N-1, rson[i] and lson[i] will be the right and left
 *  children of node i. These nodes need not be initialized. Also, dad[i]
 *  is the parent of node i. These are initialized to N, which stands for
 *  'not used.' For i = 0 to 255, rson[N+i+1] is the root of the tree for
 *  strings that begin with character i. These are initialized to N. Note
 *  there are 256 trees.
 */
static void lzss_inittree(LZSS_PACK_DATA *dat)
{
	int i;

	for (i=N+1; i<=N+256; i++)
		dat->rson[i] = N;

	for (i=0; i<N; i++)
		dat->dad[i] = N;
}



/**
 *  Inserts a string of length F, text_buf[r..r+F-1], into one of the trees
 *  (text_buf[r]'th tree) and returns the longest-match position and length
 *  via match_position and match_length. If match_length = F, then removes
 *  the old node in favor of the new one, because the old one will be
 *  deleted sooner. Note r plays double role, as tree node and position in
 *  the buffer.
 */
static void lzss_insertnode(int r, LZSS_PACK_DATA *dat)
{
	int i, p, cmp;
	unsigned char *key;
	unsigned char *text_buf = dat->text_buf;

	cmp = 1;
	key = &text_buf[r];
	p = N + 1 + key[0];
	dat->rson[r] = dat->lson[r] = N;
	dat->match_length = 0;

	for (;;) {

		if (cmp >= 0) {
			if (dat->rson[p] != N)
				p = dat->rson[p];
			else {
				dat->rson[p] = r;
				dat->dad[r] = p;
				return;
			}
		}
		else {
			if (dat->lson[p] != N)
				p = dat->lson[p];
			else {
				dat->lson[p] = r;
				dat->dad[r] = p;
				return;
			}
		}

		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)
				break;

		if (i > dat->match_length) {
			dat->match_position = p;
			if ((dat->match_length = i) >= F)
				break;
		}
	}

	dat->dad[r] = dat->dad[p];
	dat->lson[r] = dat->lson[p];
	dat->rson[r] = dat->rson[p];
	dat->dad[dat->lson[p]] = r;
	dat->dad[dat->rson[p]] = r;
	if (dat->rson[dat->dad[p]] == p)
		dat->rson[dat->dad[p]] = r;
	else
		dat->lson[dat->dad[p]] = r;
	dat->dad[p] = N;						/* remove p */
}



/**
 *  Removes a node from a tree.
 */
static void lzss_deletenode(int p, LZSS_PACK_DATA *dat)
{
	int q;

	if (dat->dad[p] == N)
		return;		/* not in tree */

	if (dat->rson[p] == N)
		q = dat->lson[p];
	else
		if (dat->lson[p] == N)
			q = dat->rson[p];
		else {
			q = dat->lson[p];
			if (dat->rson[q] != N) {
				do {
					q = dat->rson[q];
				} while (dat->rson[q] != N);
				dat->rson[dat->dad[q]] = dat->lson[q];
				dat->dad[dat->lson[q]] = dat->dad[q];
				dat->lson[q] = dat->lson[p];
				dat->dad[dat->lson[p]] = q;
			}
			dat->rson[q] = dat->rson[p];
			dat->dad[dat->rson[p]] = q;
		}

	dat->dad[q] = dat->dad[p];
	if (dat->rson[dat->dad[p]] == p)
		dat->rson[dat->dad[p]] = q;
	else
		dat->lson[dat->dad[p]] = q;

	dat->dad[p] = N;
}



/**
 *  Packs size bytes from buf, using the pack information contained in dat.
 *  Returns 0 on success.
 */
int lzss_write(PACKFILE *file, LZSS_PACK_DATA *dat, int size, unsigned char *buf, int last)
{
	int i = dat->i;
	int c = dat->c;
	int len = dat->len;
	int r = dat->r;
	int s = dat->s;
	int last_match_length = dat->last_match_length;
	int code_buf_ptr = dat->code_buf_ptr;
	unsigned char mask = dat->mask;
	int ret = 0;

	if (dat->state==2)
		goto pos2;
	else
		if (dat->state==1)
			goto pos1;

	dat->code_buf[0] = 0;
		/* code_buf[1..16] saves eight units of code, and code_buf[0] works
			as eight flags, "1" representing that the unit is an unencoded
			letter (1 byte), "0" a position-and-length pair (2 bytes).
			Thus, eight units require at most 16 bytes of code. */

	code_buf_ptr = mask = 1;

	s = 0;
	r = N - F;
	lzss_inittree(dat);

	for (len=0; (len < F) && (size > 0); len++) {
		dat->text_buf[r+len] = *(buf++);
		if (--size == 0) {
			if (!last) {
				dat->state = 1;
				goto getout;
			}
		}
		pos1:
			;
	}

	if (len == 0)
		goto getout;

	for (i=1; i <= F; i++)
		lzss_insertnode(r-i,dat);
				/* Insert the F strings, each of which begins with one or
					more 'space' characters. Note the order in which these
					strings are inserted. This way, degenerate trees will be
					less likely to occur. */

	lzss_insertnode(r,dat);
				/* Finally, insert the whole string just read. match_length
					and match_position are set. */

	do {
		if (dat->match_length > len)
			dat->match_length = len;  /* match_length may be long near the end */

		if (dat->match_length <= THRESHOLD) {
			dat->match_length = 1;	/* not long enough match: send one byte */
			dat->code_buf[0] |= mask;	  /* 'send one byte' flag */
			dat->code_buf[code_buf_ptr++] = dat->text_buf[r]; /* send uncoded */
		}
		else {
			/* send position and length pair. Note match_length > THRESHOLD */
			dat->code_buf[code_buf_ptr++] = (unsigned char) dat->match_position;
			dat->code_buf[code_buf_ptr++] = (unsigned char)
						 (((dat->match_position >> 4) & 0xF0) |
						  (dat->match_length - (THRESHOLD + 1)));
		}

		if ((mask <<= 1) == 0) {			/* shift mask left one bit */

			if ((file->is_normal_packfile) && (file->normal.passpos) &&
				 (file->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
			{
				dat->code_buf[0] ^= *file->normal.passpos;
				file->normal.passpos++;
				if (!*file->normal.passpos)
					file->normal.passpos = file->normal.passdata;
			}

			for (i=0; i<code_buf_ptr; i++)		/* send at most 8 units of */
				pack_putc(dat->code_buf[i], file);	/* code together */

			if (pack_ferror(file)) {
				ret = EOF;
				goto getout;
			}
			dat->code_buf[0] = 0;
			code_buf_ptr = mask = 1;
		}

		last_match_length = dat->match_length;

		for (i=0; (i < last_match_length) && (size > 0); i++) {
			c = *(buf++);
			if (--size == 0) {
				if (!last) {
					dat->state = 2;
					goto getout;
				}
			}
			pos2:
			lzss_deletenode(s,dat);		/* delete old strings and */
			dat->text_buf[s] = c;		/* read new bytes */
			if (s < F-1)
				dat->text_buf[s+N] = c; /* if the position is near the end of
											buffer, extend the buffer to make
											string comparison easier */
			s = (s+1) & (N-1);
			r = (r+1) & (N-1);			/* since this is a ring buffer,
											increment the position modulo N */

			lzss_insertnode(r,dat);		/* register the string in
											text_buf[r..r+F-1] */
		}

		while (i++ < last_match_length) {	/* after the end of text, */
			lzss_deletenode(s,dat);				/* no need to read, but */
			s = (s+1) & (N-1);					/* buffer may not be empty */
			r = (r+1) & (N-1);
			if (--len)
				lzss_insertnode(r,dat);
		}

	} while (len > 0);	/* until length of string to be processed is zero */

	if (code_buf_ptr > 1) {			  /* send remaining code */

		if ((file->is_normal_packfile) && (file->normal.passpos) &&
			 (file->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
		{
			dat->code_buf[0] ^= *file->normal.passpos;
			file->normal.passpos++;
			if (!*file->normal.passpos)
				file->normal.passpos = file->normal.passdata;
		}

		for (i=0; i<code_buf_ptr; i++) {
			pack_putc(dat->code_buf[i], file);
			if (pack_ferror(file)) {
				ret = EOF;
				goto getout;
			}
		}
	}

	dat->state = 0;

	getout:

	dat->i = i;
	dat->c = c;
	dat->len = len;
	dat->r = r;
	dat->s = s;
	dat->last_match_length = last_match_length;
	dat->code_buf_ptr = code_buf_ptr;
	dat->mask = mask;

	return ret;
}



/*** Decompression (reading) ***/

/**
 *  Creates an LZSS_UNPACK_DATA structure.
 */
LZSS_UNPACK_DATA *create_lzss_unpack_data(void)
{
	LZSS_UNPACK_DATA *dat;
	int c;

	if ((dat = _AL_MALLOC_ATOMIC(sizeof(LZSS_UNPACK_DATA))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	for (c=0; c < N - F; c++)
		dat->text_buf[c] = 0;

	dat->state = 0;

	return dat;
}



/**
 *  Frees an LZSS_UNPACK_DATA structure.
 */
void free_lzss_unpack_data(LZSS_UNPACK_DATA *dat)
{
	AL_ASSERT(dat);

	_AL_FREE(dat);
}



/**
 *  Unpacks from dat into buf, until either EOF is reached or s bytes have
 *  been extracted. Returns the number of bytes added to the buffer
 */
int lzss_read(PACKFILE *file, LZSS_UNPACK_DATA *dat, int s, unsigned char *buf)
{
	int i = dat->i;
	int j = dat->j;
	int k = dat->k;
	int r = dat->r;
	int c = dat->c;
	unsigned int flags = dat->flags;
	int size = 0;

	if (dat->state==2)
		goto pos2;
	else
		if (dat->state==1)
			goto pos1;

	r = N-F;
	flags = 0;

	for (;;) {
		if (((flags >>= 1) & 256) == 0) {
			if ((c = pack_getc(file)) == EOF)
				break;

			if ((file->is_normal_packfile) && (file->normal.passpos) &&
				 (file->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
			{
				c ^= *file->normal.passpos;
				file->normal.passpos++;
				if (!*file->normal.passpos)
					file->normal.passpos = file->normal.passdata;
			}

			flags = c | 0xFF00;			/* uses higher byte to count eight */
		}

		if (flags & 1) {
			if ((c = pack_getc(file)) == EOF)
				break;
			dat->text_buf[r++] = c;
			r &= (N - 1);
			*(buf++) = c;
			if (++size >= s) {
				dat->state = 1;
				goto getout;
			}
			pos1:
				;
		}
		else {
			if ((i = pack_getc(file)) == EOF)
				break;
			if ((j = pack_getc(file)) == EOF)
				break;
			i |= ((j & 0xF0) << 4);
			j = (j & 0x0F) + THRESHOLD;
			for (k=0; k <= j; k++) {
				c = dat->text_buf[(i + k) & (N - 1)];
				dat->text_buf[r++] = c;
				r &= (N - 1);
				*(buf++) = c;
				if (++size >= s) {
					dat->state = 2;
					goto getout;
				}
				pos2:
					;
			}
		}
	}

	dat->state = 0;

	getout:

	dat->i = i;
	dat->j = j;
	dat->k = k;
	dat->r = r;
	dat->c = c;
	dat->flags = flags;

	return size;
}



/**
 *  Return non-zero if the previous lzss_read() call was in the middle of
 *  unpacking a sequence of bytes into the supplied buffer, but had to suspend
 *  because the buffer wasn't big enough.
 */
int _al_lzss_incomplete_state(AL_CONST LZSS_UNPACK_DATA *dat)
{
	return dat->state == 2;
}

// vim:tabstop=4 shiftwidth=4
