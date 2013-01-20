## Convenience wrapper around raw `nimepak <nimepak.html>`_ module.
##
## The usual operation with these procs is to create a Tepak var and call
## init() on it to use it. You can call is_valid() at any time on a Tepak to
## check its validity.

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

proc is_valid*(self: Tepak): bool {.inline.} =
  ## Returns false if you shouldn't use the Tepak object.
  result = self.pak != nil

proc init*(self: var Tepak; filename, mode: string): bool =
  ## Initializes a Tepak structure opening a file.
  ##
  ## Pass the filename and mode you want to open this file in, which may
  ## contain any of the following flags:
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
  ## Returns true if the initialisation worked and you can use the structure.
  self.filename = filename
  self.mode = mode
  self.pak = pack_fopen(filename, mode)
  result = self.is_valid()

proc new_epak*(filename, mode: string): Tepak =
  ## Convenience wrapper around init to create vars.
  ##
  ## Note that you should use is_valid() on the returned structure to check if
  ## it worked, or you could fail hard.
  discard(result.init(filename, mode))

proc close*(self: var Tepak): bool {.discardable.} =
  ## Closes the Tepak.
  ##
  ## After you have closed the stream, performing operations on it will yield
  ## errors in your application (e.g. crash it) or even block your OS.
  ##
  ## Returns zero on success. On error, returns false and stores the error code
  ## in `errno`. This function can fail only when writing to files: if the file
  ## was opened in read mode, it will always succeed.
  if self.pak != nil:
    let ret = pack_fclose(self.pak)
    if ret != 0:
      echo "Problems closing epak for ", self.filename
    else:
      self.pak = nil
  result = self.pak == nil

proc seek*(self: var Tepak; offset: cint): int {.discardable.} =
  ## Seeks in the epak, returns amount of bytes seeked:
  ##
  ## Unlike the standard fseek() function, this only supports forward movements
  ## relative to the current position and in read-only streams, so don't use
  ## negative offsets. Note that seeking is very slow when reading compressed
  ## files, and so should be avoided unless you are sure that the file is not
  ## compressed.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_fseek(self.pak, offset)

proc skip_chunks*(self: var Tepak; to_skip: cuint): bool {.discardable.} =
  ## Efficient proc to skip several chunks in your packfile.
  ##
  ## This is faster than opening and closing a subchunk as you find it on disk.
  ## The function reads the hidden chunk size data and uses that to call
  ## pack_fseek instead. Pass how many chunks you want to skip, usually one.
  ##
  ## Returns true if the specified number of chunks was skipped, false
  ## otherwise, storing the error code in `errno`.
  assert self.pak != nil, "Unexpected bad epak"
  if pack_skip_chunks(self.pak, to_skip) == 0:
    result = true

proc open_chunk*(self: var Tepak; use_compression: bool): bool {.discardable.} =
  ## Opens a sub-chunk of a file.
  ##
  ## A chunk provides a logical view of part of a file, which can be compressed
  ## as an individual entity and will automatically insert and check length
  ## counts to prevent reading past the end of the chunk. If use_compression is
  ## true this will turn on compression for the sub-chunk.
  ##
  ## The data written to the chunk will be prefixed with two length
  ## counts (32-bit, a.k.a. big-endian). For uncompressed chunks these
  ## will both be set to the size of the data in the chunk. For compressed
  ## chunks (created by setting the `pack` flag), the first length will
  ## be the raw size of the chunk, and the second will be the negative
  ## size of the uncompressed data.
  ##
  ## Chunks can be nested inside each other by making repeated calls
  ## to open_chunk(). When writing a file, the compression status
  ## is inherited from the parent file, so you only need to set the
  ## compression flag if the parent is not compressed but you want to pack the
  ## chunk data. If the parent file is already open in packed mode,
  ## setting the pack flag will result in data being compressed twice:
  ## once as it is written to the chunk, and again as the chunk passes
  ## it on to the parent file.
  ##
  ## Returns true if the operation succeeded.
  assert self.pak != nil, "Unexpected bad epak"
  self.pak = pack_fopen_chunk(self.pak, if use_compression: 1 else: 0)
  result = self.is_valid()

proc close_chunk*(self: var Tepak): bool {.discardable.}  =
  ## Closes a sub-chunk of a file, previously opened with open_chunk().
  ##
  ## Returns false if there was some error.
  assert self.pak != nil, "Unexpected bad epak"
  self.pak = pack_fclose_chunk(self.pak)
  result = self.is_valid()

proc getc*(self: var Tepak): cint =
  ## Returns the next character from the stream, or EOF (-1) if the end of the
  ## file has been reached.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_getc(self.pak)
  #assert pack_ferror(self.pak) == 0, "Error pending"

proc putc*(self: var Tepak; value: cint) =
  ## Puts a character in the stream.
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_putc(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc eof*(self: var Tepak): bool =
  ## Returns true as soon as you reach the end of the file.
  ##
  ## It does not wait for you to attempt to read beyond the end of the file,
  ## contrary to the ISO C feof() function. The only way to know whether you
  ## have read beyond the end of the file is to check the return value of the
  ## read operation you use (and be wary of \*getl() procs as EOF is also a
  ## valid return value with these).
  ##
  ## Returns true if you are at the end of a file, false otherwise.
  assert self.pak != nil, "Unexpected bad epak"
  result = (pack_feof(self.pak) != 0)

proc iget16*(self: var Tepak): cint =
  ## Reads a 16 bit word from the file, using intel/little-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_igetw(self.pak)
  #assert pack_ferror(self.pak) == 0, "Error pending"

proc iget32*(self: var Tepak): clong =
  ## Reads a 32 bit long from the file, using intel/little-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_igetl(self.pak)
  #assert pack_ferror(self.pak) == 0, "Error pending"

proc iput16*(self: var Tepak; value: cint) =
  ## Writes a 16 bit word to the file, using intel/little-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_iputw(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc iput32*(self: var Tepak; value: clong) =
  ## Writes a 16 bit long to the file, using intel/little-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_iputl(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc mget16*(self: var Tepak): cint =
  ## Reads a 16 bit word from the file, using motorola/big-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_mgetw(self.pak)
  #assert pack_ferror(self.pak) == 0, "Error pending"

proc mget32*(self: var Tepak): clong =
  ## Reads a 32 bit long from the file, using motorola/big-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_mgetl(self.pak)
  #assert pack_ferror(self.pak) == 0, "Error pending"

proc mput16*(self: var Tepak; value: cint) =
  ## Writes a 16 bit word to the file, using motorola/big-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_mputw(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc mput32*(self: var Tepak; value: clong) =
  ## Writes a 32 bit long to the file, using motorola/big-endian byte ordering.
  assert self.pak != nil, "Unexpected bad epak"
  let ret = pack_mputl(value, self.pak)
  assert ret == value, "Didn't write input value"
  assert pack_ferror(self.pak) == 0, "Error pending"

proc read*(self: var Tepak; dest: pointer; length: clong): clong =
  ## Reads length bytes from the packfile and stores them at dest.
  ##
  ## Returns the number of bytes read, which will be less than `length` if EOF
  ## is reached or an error occurs. Error codes are stored in `errno`.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_fread(dest, length, self.pak)
  #assert result == length, "Didn't read expected char count"

proc read*(self: var Tepak; length: clong): string =
  ## Reads length bytes from the packfile and returns them as a string.
  ##
  ## The returned string might be shorter than expected if EOF is reached or an
  ## error occurs.
  assert self.pak != nil, "Unexpected bad epak"
  result = newString(length)
  let read_length = pack_fread(cstring(result), length, self.pak)
  result.setLen(read_length)
  #assert read_length == length, "Didn't read expected char count"

proc write*(self: var Tepak; src: pointer; length: clong):
    clong {.discardable.} =
  ## Writes length bytes to the packfile from src.
  ##
  ## Returns the number of bytes written, which will be less than length if an
  ## error occurs. Error codes are stored in errno.
  assert self.pak != nil, "Unexpected bad epak"
  result = pack_fwrite(src, length, self.pak)
  #assert result == length, "Didn't write all bytes?"
  #assert pack_ferror(self.pak) == 0, "Error pending"

proc write*(self: var Tepak; text: string; append_terminator = true):
    clong {.discardable.} =
  ## Writes the specified nimrod string to the packfile.
  ##
  ## By default the append_terminator is true, so the write operation will
  ## include the C NULL terminator for strings. If you pass false no byte
  ## terminator will be appended.
  ##
  ## Returns the number of bytes written, which will be less than length if an
  ## error occurs. Error codes are stored in errno.
  assert self.pak != nil, "Unexpected bad epak"
  let text_len = if append_terminator: text.len + 1 else: text.len
  result = pack_fwrite(cstring(text), clong(text_len), self.pak)
  assert result == text_len, "Didn't write all bytes?"
  assert pack_ferror(self.pak) == 0, "Error pending"
