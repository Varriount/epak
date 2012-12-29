// The epak.h was processed with c2nim and tweaked.

#ifdef C2NIM
#	header "epak.h"
#	typeprefixes
#	skipcomments
#endif

#define PACKFILE_FLAG_WRITE      1
#define PACKFILE_FLAG_PACK       2
#define PACKFILE_FLAG_CHUNK      4
#define PACKFILE_FLAG_EOF        8
#define PACKFILE_FLAG_ERROR      16
#define PACKFILE_FLAG_OLD_CRYPT  32
#define PACKFILE_FLAG_EXEDAT     64

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

typedef struct PACKFILE_VTABLE_t PACKFILE_VTABLE;
typedef struct PACKFILE_t PACKFILE;
typedef struct LZSS_PACK_DATA_t LZSS_PACK_DATA;
typedef struct LZSS_UNPACK_DATA_t LZSS_UNPACK_DATA;


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

	struct _al_normal_packfile_details normal;
};


struct PACKFILE_VTABLE_t
{
	int (* pf_fclose)  (void *userdata);
	int (* pf_getc)  (void *userdata);
	int (* pf_ungetc)  (int c, void *userdata);
	long (* pf_fread)  (void *p, long n, void *userdata);
	int (* pf_putc)  (int c, void *userdata);
	long (* pf_fwrite)  (const void *p, long n, void *userdata);
	int (* pf_fseek)  (void *userdata, int offset);
	int (* pf_feof)  (void *userdata);
	int (* pf_ferror)  (void *userdata);
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
