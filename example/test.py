#!/usr/bin/env python
# -*- encoding: utf8 -*- vim: tabstop=4 shiftwidth=4 encoding=utf-8
"""Tests in python that the datafile functions work."""

import epak
import hashlib
import os
import os.path
import sys

WITH_PASS_EPAK = "with pass.epak"
NO_PASS_EPAK = "no pass.epak"
PY_WITH_PASS_EPAK = "py %s" % WITH_PASS_EPAK
PY_NO_PASS_EPAK = "py %s" % NO_PASS_EPAK
PASSWORD = "12358 Dummy password"
STR1 = "00000000000000000000 This is a test string 1, don't hurt me!"
STR2 = "This is a test string 2, don't hurt me!"
STRINGS = [STR1, STR2]


def precondition_check():
	"""f() -> None

	Disallow running the program if the C version generated files are not there.
	"""
	for filename in [WITH_PASS_EPAK, NO_PASS_EPAK]:
		if not os.isfile(filename):
			print "Missing %r" % filename
			sys.exit(1)


def pack_test(filename):
	"""f(string) -> None

	Writes into the specified filename the data for the test.
	"""
	pak = epak.open(filename, "w!")
	assert pak, "Error creating test file"

	# Test writting the string a a packfile, first uncompressed.
	for compressed in range(2):
		pak = epak.open_chunk(pak, compressed)
		assert pak, "Error opening subchunk!"
		# Repeat five times to allow for compression to kick in.
		for times in range(5):
			ret = epak.write(STRINGS[compressed], pak)
			assert len(STRINGS[compressed]) == ret

		pak = epak.close_chunk(pak)
		assert pak

	if epak.close(pak):
		assert False, "Error closing packfile!"


def read_skip_test(filename):
	"""f(string) -> None

	Verifies that data can be read and skipped inside chunks.
	"""
	pak = epak.open(filename, "rp")
	assert pak, "Couldn't read packfile"

	# Open and close the first chunk, skipping. Note we ask for compression.
	pak = epak.open_chunk(pak, 1)
	assert pak, "Couldn't open first subchunk"
	pak = epak.close_chunk(pak)
	assert pak, "Couldn't skip first subchunk"

	pak = epak.open_chunk(pak, 1)
	assert pak, "Couldn't open second subchunk"
	buf = epak.read(len(STR2), pak)
	assert len(buf) == len(STR2)
	assert buf == STR2

	closing = epak.close(pak)
	assert not closing, "Couldn't force close!"


def get_md5(filename):
	"""f(string) -> string

	Returns the md5 hash of the specified file.
	"""
	md5 = hashlib.md5()
	input = open(filename, "rb")
	md5.update(input.read())
	input.close()
	return md5.hexdigest()


def main():
	print "Testing python version"

	epak.set_password(PASSWORD)
	read_skip_test(WITH_PASS_EPAK)

	epak.set_password("")
	read_skip_test(NO_PASS_EPAK)

	epak.set_password(PASSWORD)
	pack_test(PY_WITH_PASS_EPAK)

	epak.set_password("")
	pack_test(PY_NO_PASS_EPAK)

	assert get_md5(WITH_PASS_EPAK) == get_md5(PY_WITH_PASS_EPAK)
	assert get_md5(NO_PASS_EPAK) == get_md5(PY_NO_PASS_EPAK)

if "__main__" == __name__:
	main()
