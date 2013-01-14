# Convenience wrapper around raw nimepak module.

import nimepak

export nimepak.F_BUF_SIZE
export nimepak.F_PACK_MAGIC
export nimepak.F_NOPACK_MAGIC
export nimepak.F_EXE_MAGIC
export nimepak.PACKFILE_FLAG_WRITE
export nimepak.PACKFILE_FLAG_PACK
export nimepak.PACKFILE_FLAG_CHUNK
export nimepak.PACKFILE_FLAG_EOF
export nimepak.PACKFILE_FLAG_ERROR
export nimepak.PACKFILE_FLAG_OLD_CRYPT
export nimepak.PACKFILE_FLAG_EXEDAT
export nimepak.F_READ
export nimepak.F_WRITE
export nimepak.F_READ_PACKED
export nimepak.F_WRITE_PACKED
export nimepak.F_WRITE_NOPACK
export nimepak.packfile_password

type
  Tepak* = object
    filename*: string
    mode*: string
    pak*: PPACKFILE

proc init*(self: var Tepak; filename, mode: string) =
  ## Doesn't check if the opening worked, use is_valid to check later.
  self.filename = filename
  self.mode = mode
  self.pak = pack_fopen(filename, mode)

proc new_epak*(filename, mode: string): Tepak =
  ## Convenience wrapper around init to create vars.
  result.init(filename, mode)

proc is_valid*(self: Tepak): bool {.inline.} =
  result = self.pak != nil

proc close*(self: var Tepak): bool {.discardable.} =
  if self.pak != nil:
    let ret = pack_fclose(self.pak)
    if ret != 0:
      echo "Problems closing epak for ", self.filename
    else:
      self.pak = nil
  result = self.pak == nil

proc seek*(self: var Tepak; offset: cint): int {.discardable.} =
  ## Seeks in the epak, returns amount of bytes seeked:
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_fseek(self.pak, offset)

proc skip_chunks*(self: var Tepak; to_skip: cuint): cint =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_skip_chunks(self.pak, to_skip)

proc open_chunk*(self: var Tepak; use_compression: bool) =
  assert self.pak != nil, "Unexpected bad epak"
  self.pak = pack_fopen_chunk(self.pak, if use_compression: 1 else: 0)
  assert self.pak != nil, "Couldn't open a subchunk!"

proc close_chunk*(self: var Tepak) =
  ## Rather than checking the return value of the close action, use is_valid
  assert self.pak != nil, "Unexpected bad epak"
  self.pak = pack_fclose_chunk(self.pak)
  assert self.pak != nil, "Couldn't close a subchunk!"

proc getc*(self: var Tepak): cint =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_getc(self.pak)
  assert pack_ferror(self.pak) == 0, "Error pending"

proc putc*(self: var Tepak; value: cint) =
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_putc(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc eof*(self: var Tepak): bool =
  assert self.pak != nil, "Unexpected bad epak"
  result = (pack_feof(self.pak) != 0)

proc igetw*(self: var Tepak): cint =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_igetw(self.pak)
  assert pack_ferror(self.pak) == 0, "Error pending"

proc igetl*(self: var Tepak): clong =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_igetl(self.pak)
  assert pack_ferror(self.pak) == 0, "Error pending"

proc iputw*(self: var Tepak; value: cint) =
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_iputw(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc iputl*(self: var Tepak; value: clong) =
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_iputl(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc mgetw*(self: var Tepak): cint =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_mgetw(self.pak)
  assert pack_ferror(self.pak) == 0, "Error pending"

proc mgetl*(self: var Tepak): clong =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_mgetl(self.pak)
  assert pack_ferror(self.pak) == 0, "Error pending"

proc mputw*(self: var Tepak; value: cint) =
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_mputw(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc mputl*(self: var Tepak; value: clong) =
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_mputl(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc read*(self: var Tepak; dest: pointer; length: clong): clong =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_fread(dest, length, self.pak)
  assert result == length, "Didn't read expected char count"

proc read*(self: var Tepak; length: clong): string =
  assert self.pak != nil, "Unexpected bad epak"
  result = newString(length)
  let read_length = pack_fread(cstring(result), length, self.pak)
  result.setLen(read_length)
  assert read_length == length, "Didn't read expected char count"

proc write*(self: var Tepak; src: pointer; length: clong):
    clong {.discardable.} =
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_fwrite(src, length, self.pak)
  assert result == length, "Didn't write all bytes?"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc write*(self: var Tepak; text: string; append_terminator = true):
    clong {.discardable.} =
  ## Convenience wrapper, writes the string including null terminator by default
  assert self.pak != nil, "Unexpected bad epak"
  let text_len = if append_terminator: text.len + 1 else: text.len
  result = pack_fwrite(cstring(text), clong(text_len), self.pak)
  assert result == text_len, "Didn't write all bytes?"
  assert pack_ferror(self.pak) == 0, "Error pending"
