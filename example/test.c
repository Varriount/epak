/*
 * Exercises the pack* functions to verify they are working.
 */

#include "epak.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PASSWORD		"12358 Dummy password"

/* The repetition of zeros highlights in the generated encrypted file that the
 * encryption doesn't use CBC mode, and thus by knowing that a list of zeroes
 * is contigous, the attacker could guess xor ecryption easily.
 */
const char *test_string[] = {"00000000000000000000 This is a test string 1, don't hurt me!",
	"This is a test string 2, don't hurt me!"};


// Tests writting a few strings into a file, both with and without compression.
void pack_test(const char *filename)
{
	PACKFILE *pak = pack_fopen(filename, F_WRITE_NOPACK);
	assert(pak && "Error creating test file");

	// Test writing the string as a packfile, first uncompressed.
	int compressed, times;
	for (compressed = 0; compressed < 2; compressed++) {
		pak = pack_fopen_chunk(pak, compressed);
		assert(pak && "Error opening subchunk!");
		// Repeat five times to allow for compression to kick in.
		for (times = 0; times < 5; times++) {
			const unsigned long ret = pack_fwrite(test_string[compressed],
				strlen(test_string[compressed]), pak);
			assert(ret == strlen(test_string[compressed]));
		}
		pak = pack_fclose_chunk(pak);
		assert(pak);
	}

	if (pack_fclose(pak)) {
		assert(0 && "Error closing packfile!");
	}
}

// Similar to pack_test(), but only reads skipping the first chunk.
void read_skip_test(const char *filename)
{
	char buf[255];
	const char *s = test_string[1];

	PACKFILE *pak = pack_fopen(filename, F_READ_PACKED);
	assert(pak && "Couldn't read packfile");

	// Open and close the first chunk, skipping. Note we ask for compression.
	pak = pack_fopen_chunk(pak, 1);
	assert(pak && "Couldn't open first subckunk");
	pak = pack_fclose_chunk(pak);
	assert(pak && "Couldn't skip first subchunk");

	pak = pack_fopen_chunk(pak, 1);
	assert(pak && "Couldn't open second subckunk");
	const unsigned long ret = pack_fread(buf, strlen(s), pak);
	assert(ret == strlen(s));

	assert(!strncmp(buf, s, strlen(s)));
	// Force closing all packfiles, even though we aren't finished.
	const int closing = pack_fclose(pak);
	assert(!closing && "Couldn't force close!");

	// Now repeat the second read skipping the first chunk.
	pak = pack_fopen(filename, F_READ_PACKED);
	if (pack_skip_chunks(pak, 1)) {
		assert(0 && "Couldn't skip first chunk");
	}

	pak = pack_fopen_chunk(pak, 1);
	assert(pak && "Couldn't open second subckunk");
	const unsigned long ret2 = pack_fread(buf, strlen(s), pak);
	assert(ret2 == strlen(s));

	assert(!strncmp(buf, s, strlen(s)));
	pack_fclose(pak);
}

int main(void)
{
	printf("Testing epak functions.\n");

	packfile_password(PASSWORD);
	pack_test("with pass.epak");

	packfile_password(0);
	pack_test("no pass.epak");

	packfile_password(PASSWORD);
	read_skip_test("with pass.epak");

	packfile_password(0);
	read_skip_test("no pass.epak");

	printf("Test finished.\n");

	return 0;
}

// vim:tabstop=4 shiftwidth=4
