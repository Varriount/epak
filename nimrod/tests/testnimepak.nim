import nimepak

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
  var pak = pack_fopen(filename, F_WRITE_NOPACK)
  assert pak != nil, "Error creating test file"

  # Test writing the string as a packfile, first uncompressed.
  for compressed in 0..1:
    pak = pack_fopen_chunk(pak, cint(compressed))
    assert pak != nil, "Error opening subchunk!"
    # Repeat five times to allow for compression to kick in.
    for times in 0..4:
      let ret = pack_fwrite(cstring(test_string[compressed]),
        clong(len(test_string[compressed])), pak)
      assert ret == len(test_string[compressed]), "Not enough bytes written?"
    pak = pack_fclose_chunk(pak)
    assert pak != nil, "Unexpected error"

  if pack_fclose(pak) != 0:
    quit("Error closing packfile!", 1)

  echo "finished!"


proc read_skip_test(filename: string) =
  # Similar to pack_test(), but only reads skipping the first chunk
  let s = test_string[1]
  var
    buf: array[0..254, char]
    pak = pack_fopen(filename, F_READ_PACKED)

  assert pak != nil, "Couldn't read packfile"

  # Open and close the first chunk, skipping. Note we ask for compression.
  pak = pack_fopen_chunk(pak, 1)
  assert pak != nil, "Couldn't open first subchunk"
  pak = pack_fclose_chunk(pak)
  assert pak != nil, "Couldn't skip first subchunk"

  pak = pack_fopen_chunk(pak, 1);
  assert pak != nil, "Couldn't open second subckunk"
  let ret = pack_fread(cstring(buf), clong(len(s)), pak)
  assert ret == len(s), "Didn't read expected char count"

  for i in 0..len(s)-1:
    assert buf[i] == s[i], "Bad character comparison"

  #	Force closing all packfiles, even though we aren't finished.
  let closing = pack_fclose(pak)
  assert closing == 0, "Couldn't force close!"

  # Now repeat the second read skipping the first chunk.
  pak = pack_fopen(filename, F_READ_PACKED)
  if pack_skip_chunks(pak, 1) != 0:
    quit("Couldn't skip first chunk", 1)

  pak = pack_fopen_chunk(pak, 1)
  assert pak != nil, "Couldn't open second subckunk"
  let ret2 = pack_fread(cstring(buf), clong(len(s)), pak)
  assert ret2 == len(s), "Didn't read expected char count"

  for i in 0..len(s)-1:
    assert buf[i] == s[i], "Bad character comparison"
  discard(pack_fclose(pak))


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
