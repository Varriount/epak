#!/usr/bin/env python
# -*- encoding: utf8 -*- vim: tabstop=4 shiftwidth=4 encoding=utf-8
"""Tests in python that the datafile functions work."""

import epak
import os
import os.path
import sys

WITH_PASS_EPAK = "with pass.epak"
NO_PASS_EPAK = "no pass.epak"
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


def main():
	print "Testing python read skipping code."

	epak.set_password(PASSWORD)
	read_skip_test(WITH_PASS_EPAK);

	epak.set_password("")
	read_skip_test(NO_PASS_EPAK);

if "__main__" == __name__:
	main()
