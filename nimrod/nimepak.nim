## The nimepak module provides a nimrod binding against the C epak library (see
## https://github.com/gradha/epak ). This library is a subset of Allegro's
## file and compression routines (see
## http://alleg.sourceforge.net/stabledocs/en/alleg030.html ). The `nimepakoo
## <nimepakoo.html>`_ object oriented module provides a more convenient way to
## use these procs, so go read its documentation instead.

# The bundled.c file is a pregenerated file result of taking epak's C source
# and concatenating it into a single file. This allows us to use Nimrod's
# compile macro to statically embed the source on every nimrod compilation, and
# thus avoid problems with system-wide installed libraries. It also helps
# versioning.
{.compile: "bundled.c".}

const
  PACKFILE_FLAG_WRITE* = 1
  PACKFILE_FLAG_PACK* = 2
  PACKFILE_FLAG_CHUNK* = 4
  PACKFILE_FLAG_EOF* = 8
  PACKFILE_FLAG_ERROR* = 16
  PACKFILE_FLAG_OLD_CRYPT* = 32
  PACKFILE_FLAG_EXEDAT* = 64
  F_READ* = "r"
  F_WRITE* = "w"
  F_READ_PACKED* = "rp"
  F_WRITE_PACKED* = "wp"
  F_WRITE_NOPACK* = "w!"


const
  F_BUF_SIZE* = 4096
  F_PACK_MAGIC* = 0x736C6821
  F_NOPACK_MAGIC* = 0x736C682E
  F_EXE_MAGIC* = 0x736C682B

type
  TPACKFILE_VTABLE* {.importc: "struct TPACKFILE_VTABLE_t".} = object
  TPACKFILE* {.importc: "struct TPACKFILE_t".} = object
  PPACKFILE* = ptr TPACKFILE
  TLZSS_PACK_DATA* {.importc: "struct TLZSS_PACK_DATA_t".} = object
  TLZSS_UNPACK_DATA* {.importc: "struct TLZSS_UNPACK_DATA_t".} = object
  T_al_normal_packfile_details* {.pure, final,
                                  importc: "_al_normal_packfile_details",
                                  .} = object
    hndl* {.importc: "hndl".}: cint
    flags* {.importc: "flags".}: cint
    buf_pos* {.importc: "buf_pos".}: ptr cuchar
    buf_size* {.importc: "buf_size".}: cint
    todo* {.importc: "todo".}: clong
    parent* {.importc: "parent".}: ptr TPACKFILE
    pack_data* {.importc: "pack_data".}: ptr TLZSS_PACK_DATA
    unpack_data* {.importc: "unpack_data".}: ptr TLZSS_UNPACK_DATA
    filename* {.importc: "filename".}: cstring
    passdata* {.importc: "passdata".}: cstring
    passpos* {.importc: "passpos".}: cstring
    buf* {.importc: "buf".}: array[0..F_BUF_SIZE - 1, cuchar]

  TPACKFILE_t* {.pure, final, importc: "PACKFILE_t".} = object
    vtable* {.importc: "vtable".}: ptr TPACKFILE_VTABLE
    userdata* {.importc: "userdata".}: pointer
    is_normal_packfile* {.importc: "is_normal_packfile".}: cint
    normal* {.importc: "normal".}: T_al_normal_packfile_details

  TPACKFILE_VTABLE_t* {.pure, final, importc: "PACKFILE_VTABLE_t",
                        .} = object
    pf_fclose* {.importc: "pf_fclose".}: proc (userdata: pointer): cint
    pf_getc* {.importc: "pf_getc".}: proc (userdata: pointer): cint
    pf_ungetc* {.importc: "pf_ungetc".}: proc (c: cint; userdata: pointer): cint
    pf_fread* {.importc: "pf_fread".}: proc (p: pointer; n: clong;
        userdata: pointer): clong
    pf_putc* {.importc: "pf_putc".}: proc (c: cint; userdata: pointer): cint
    pf_fwrite* {.importc: "pf_fwrite".}: proc (p: pointer; n: clong;
        userdata: pointer): clong
    pf_fseek* {.importc: "pf_fseek".}: proc (userdata: pointer; offset: cint): cint
    pf_feof* {.importc: "pf_feof".}: proc (userdata: pointer): cint
    pf_ferror* {.importc: "pf_ferror".}: proc (userdata: pointer): cint


proc packfile_password*(password: cstring) {.importc: "packfile_password".}
  ## Sets the global packfile password, replacing
  ## Sets the encryption password to be used for all read/write operations on
  ## files opened in future using Allegro's packfile functions (whether they
  ## are compressed or not), including all the save, load and config routines.
  ## Files written with an encryption password cannot be read unless the same
  ## password is selected, so be careful: if you forget the key, nobody can
  ## make your data come back again! Pass NULL or an empty string to return to
  ## the normal, non-encrypted mode. If you are using this function to prevent
  ## people getting access to your datafiles, be careful not to store an
  ## obvious copy of the password in your executable: if there are any strings
  ## like "I'm the password for the datafile", it would be fairly easy to get
  ## access to your data :-)
  ##
  ## Note #1: when writing a packfile, you can change the password to whatever
  ## you want after opening the file, without affecting the write operation. On
  ## the contrary, when writing a sub-chunk of a packfile, you must make sure
  ## that the password that was active at the time the sub-chunk was opened is
  ## still active before closing the sub-chunk. This is guaranteed to be true
  ## if you didn't call the packfile_password() routine in the meantime. Read
  ## operations, either on packfiles or sub-chunks, have no such restriction.
  ##
  ## Note #2: as explained above, the password is used for all read/write
  ## operations on files. As a rule of thumb, always call
  ## packfile_password(NULL) when you are done with operations on packfiles.

proc pack_fopen*(filename: cstring; mode: cstring): ptr TPACKFILE {.
    importc: "pack_fopen"}
  ## Opens a file according to mode, which may contain any of the flags:
  ##
  ## `r` - open file for reading.
  ##
  ## `w` - open file for writing, overwriting any existing data.
  ##
  ## `p` - open file in packed mode. Data will be compressed as it is written
  ## to the file, and automatically uncompressed during read operations. Files
  ## created in this mode will produce garbage if they are read without this
  ## flag being set.
  ##
  ## `!` - open file for writing in normal, unpacked mode, but add the value
  ## F_NOPACK_MAGIC to the start of the file, so that it can later be opened in
  ## packed mode and Epak will automatically detect that the data does not
  ## need to be decompressed.
  ##
  ## Instead of these flags, one of the constants F_READ, F_WRITE,
  ## F_READ_PACKED, F_WRITE_PACKED or F_WRITE_NOPACK may be used as the mode
  ## parameter.
  ##
  ## On success returns a pointer to a C TPACKFILE structure. Returns NULL on
  ## failure, storing the error code in `errno`.

proc pack_fopen_vtable*(vtable: ptr TPACKFILE_VTABLE; userdata: pointer): ptr TPACKFILE {.
    importc: "pack_fopen_vtable"}
proc pack_fclose*(f: ptr TPACKFILE): cint {.importc: "pack_fclose".}
proc pack_fseek*(f: ptr TPACKFILE; offset: cint): cint {.importc: "pack_fseek".}
proc pack_skip_chunks*(f: ptr TPACKFILE; num_chunks: cuint): cint {.
    importc: "pack_skip_chunks".}
proc pack_fopen_chunk*(f: ptr TPACKFILE; pack: cint): ptr TPACKFILE {.
    importc: "pack_fopen_chunk".}
proc pack_fclose_chunk*(f: ptr TPACKFILE): ptr TPACKFILE {.
    importc: "pack_fclose_chunk".}
proc pack_getc*(f: ptr TPACKFILE): cint {.importc: "pack_getc".}
proc pack_putc*(c: cint; f: ptr TPACKFILE): cint {.importc: "pack_putc".}
proc pack_feof*(f: ptr TPACKFILE): cint {.importc: "pack_feof".}
proc pack_ferror*(f: ptr TPACKFILE): cint {.importc: "pack_ferror".}
proc pack_igetw*(f: ptr TPACKFILE): cint {.importc: "pack_igetw".}
proc pack_igetl*(f: ptr TPACKFILE): clong {.importc: "pack_igetl".}
proc pack_iputw*(w: cint; f: ptr TPACKFILE): cint {.importc: "pack_iputw".}
proc pack_iputl*(l: clong; f: ptr TPACKFILE): clong {.importc: "pack_iputl".}
proc pack_mgetw*(f: ptr TPACKFILE): cint {.importc: "pack_mgetw".}
proc pack_mgetl*(f: ptr TPACKFILE): clong {.importc: "pack_mgetl".}
proc pack_mputw*(w: cint; f: ptr TPACKFILE): cint {.importc: "pack_mputw".}
proc pack_mputl*(l: clong; f: ptr TPACKFILE): clong {.importc: "pack_mputl".}
proc pack_fread*(p: pointer; n: clong; f: ptr TPACKFILE): clong {.
    importc: "pack_fread".}
proc pack_fwrite*(p: pointer; n: clong; f: ptr TPACKFILE): clong {.
    importc: "pack_fwrite".}
proc pack_ungetc*(c: cint; f: ptr TPACKFILE): cint {.importc: "pack_ungetc".}
