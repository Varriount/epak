import nimepakoo

const
  PASSWORD = "12358 Dummy password"

  # The repetition of zeros highlights in the generated encrypted file that the
  # encryption doesn't use CBC mode, and thus by knowing that a list of zeroes
  # is contigous, the attacker could guess xor ecryption easily.
  test_string = [
    "00000000000000000000 This is a test string 1, don't hurt me!",
    "This is a test string 2, don't hurt me!"]


proc pack_test(filename: string) =
  # Tests writting a few strings into a file, both with and without
  # compression.
  var pak : Tepak
  pak.init(filename, F_WRITE_NOPACK)
  assert pak.is_valid

  # Test writing the string as a packfile, first uncompressed.
  for compressed in 0..1:
    pak.open_chunk(compressed > 0)
    # Repeat five times to allow for compression to kick in.
    for times in 0..4:
      let ret = pak.write(cstring(test_string[compressed]),
        clong(len(test_string[compressed])))
      assert ret == len(test_string[compressed]), "Not enough bytes written?"
    pak.close_chunk
    assert pak.is_valid, "Unexpected error"

  if not pak.close():
    quit("Error closing packfile!", 1)


proc read_skip_test(filename: string) =
  # Similar to pack_test(), but only reads skipping the first chunk
  let s = test_string[1]
  var
    buf: array[0..254, char]
    pak: Tepak

  pak.init(filename, F_READ_PACKED)
  assert pak.is_valid, "Couldn't read packfile"

  # Open and close the first chunk, skipping. Note we ask for compression.
  pak.open_chunk(true)
  assert pak.is_valid, "Couldn't open first subchunk"
  pak.close_chunk
  assert pak.is_valid, "Couldn't skip first subchunk"

  pak.open_chunk(true)
  assert pak.is_valid, "Couldn't open second subckunk"
  let ret = pak.read(cstring(buf), clong(len(s)))
  assert ret == len(s), "Didn't read expected char count"

  for i in 0..len(s)-1:
    assert buf[i] == s[i], "Bad character comparison"

  #	Force closing all packfiles, even though we aren't finished.
  let closing = pak.close
  assert closing, "Couldn't force close!"

  # Now repeat the second read skipping the first chunk.
  pak.init(filename, F_READ_PACKED)
  assert pak.is_valid, "Couldn't open pak for reading"
  if pak.skip_chunks(1) != 0:
    quit("Couldn't skip first chunk", 1)

  pak.open_chunk(true)
  assert pak.is_valid, "Couldn't open second subckunk"
  let ret2 = pak.read(cstring(buf), clong(len(s)))
  assert ret2 == len(s), "Didn't read expected char count"

  for i in 0..len(s)-1:
    assert buf[i] == s[i], "Bad character comparison"
  pak.close


when isMainModule:
  echo "Testing nimepak procs"
  packfile_password(PASSWORD)
  pack_test("with pass.epak")

  packfile_password(nil);
  pack_test("no pass.epak")

  packfile_password(PASSWORD)
  read_skip_test("with pass.epak")

  packfile_password(nil);
  read_skip_test("no pass.epak")

  echo "Testing finished"
