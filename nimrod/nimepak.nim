# The bundled.c file is a pregenerated file which takes the normal C source and
# concatenates it into a single file. This allows us to use Nimrod's compile
# macro to statically embed the source on every nimrod compilation, and thus
# avoid problems with system-wide installed libraries. It also helps
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
                                  header: "epak.h".} = object
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

  TPACKFILE_t* {.pure, final, importc: "PACKFILE_t", header: "epak.h".} = object
    vtable* {.importc: "vtable".}: ptr TPACKFILE_VTABLE
    userdata* {.importc: "userdata".}: pointer
    is_normal_packfile* {.importc: "is_normal_packfile".}: cint
    normal* {.importc: "normal".}: T_al_normal_packfile_details

  TPACKFILE_VTABLE_t* {.pure, final, importc: "PACKFILE_VTABLE_t",
                        header: "epak.h".} = object
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


proc packfile_password*(password: cstring) {.importc: "packfile_password",
    header: "epak.h".}
proc pack_fopen*(filename: cstring; mode: cstring): ptr TPACKFILE {.
    importc: "pack_fopen", header: "epak.h".}
proc pack_fopen_vtable*(vtable: ptr TPACKFILE_VTABLE; userdata: pointer): ptr TPACKFILE {.
    importc: "pack_fopen_vtable", header: "epak.h".}
proc pack_fclose*(f: ptr TPACKFILE): cint {.importc: "pack_fclose",
    header: "epak.h".}
proc pack_fseek*(f: ptr TPACKFILE; offset: cint): cint {.importc: "pack_fseek",
    header: "epak.h".}
proc pack_skip_chunks*(f: ptr TPACKFILE; num_chunks: cuint): cint {.
    importc: "pack_skip_chunks", header: "epak.h".}
proc pack_fopen_chunk*(f: ptr TPACKFILE; pack: cint): ptr TPACKFILE {.
    importc: "pack_fopen_chunk", header: "epak.h".}
proc pack_fclose_chunk*(f: ptr TPACKFILE): ptr TPACKFILE {.
    importc: "pack_fclose_chunk", header: "epak.h".}
proc pack_getc*(f: ptr TPACKFILE): cint {.importc: "pack_getc", header: "epak.h".}
proc pack_putc*(c: cint; f: ptr TPACKFILE): cint {.importc: "pack_putc",
    header: "epak.h".}
proc pack_feof*(f: ptr TPACKFILE): cint {.importc: "pack_feof", header: "epak.h".}
proc pack_ferror*(f: ptr TPACKFILE): cint {.importc: "pack_ferror",
    header: "epak.h".}
proc pack_igetw*(f: ptr TPACKFILE): cint {.importc: "pack_igetw",
    header: "epak.h".}
proc pack_igetl*(f: ptr TPACKFILE): clong {.importc: "pack_igetl",
    header: "epak.h".}
proc pack_iputw*(w: cint; f: ptr TPACKFILE): cint {.importc: "pack_iputw",
    header: "epak.h".}
proc pack_iputl*(l: clong; f: ptr TPACKFILE): clong {.importc: "pack_iputl",
    header: "epak.h".}
proc pack_mgetw*(f: ptr TPACKFILE): cint {.importc: "pack_mgetw",
    header: "epak.h".}
proc pack_mgetl*(f: ptr TPACKFILE): clong {.importc: "pack_mgetl",
    header: "epak.h".}
proc pack_mputw*(w: cint; f: ptr TPACKFILE): cint {.importc: "pack_mputw",
    header: "epak.h".}
proc pack_mputl*(l: clong; f: ptr TPACKFILE): clong {.importc: "pack_mputl",
    header: "epak.h".}
proc pack_fread*(p: pointer; n: clong; f: ptr TPACKFILE): clong {.
    importc: "pack_fread", header: "epak.h".}
proc pack_fwrite*(p: pointer; n: clong; f: ptr TPACKFILE): clong {.
    importc: "pack_fwrite", header: "epak.h".}
proc pack_ungetc*(c: cint; f: ptr TPACKFILE): cint {.importc: "pack_ungetc",
    header: "epak.h".}
