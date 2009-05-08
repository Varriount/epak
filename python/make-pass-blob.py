#!/usr/bin/env python
# -*- encoding: utf8 -*- vim: tabstop=4 shiftwidth=4 encoding=utf-8
"""Generates a big ascii safe blob as a password string."""

import random
import string
import sys

MAX_LINE = 40

valid = {}
for letter in string.ascii_letters:
	valid[ord(letter)] = 1
for letter in string.digits:
	valid[ord(letter)] = 1


def textualize(blob):
	"""f([int, ...]) -> string

	Ascii safe string for representation.
	"""
	letters = []
	for value in blob:
		if value in valid:
			letters.append(chr(value))
		else:
			letters.append("\\x%02x" % value)
	return "".join(letters)


def make_blob(length):
	"""f(int) -> string

	Returns a random binary blob as list of integers.
	"""
	assert length > 0
	values = []
	for iteration in range(length):
		values.append(random.randint(1, 255))
	return values


def split_blob(text):
	"""f(string) -> [string, ...]

	Splits a chunk of text in lines no longer than MAX_LINE.
	"""
	lines = []
	while text:
		lines.append(text[:MAX_LINE])
		text = text[MAX_LINE:]
	return lines


def main():
	try:
		param1 = sys.argv[1]
		length = int(param1)
	except (IndexError, ValueError):
		print "Pass the length of the ascii blob in bytes as first parameter."
		return

	blob = make_blob(length)
	print "\n// Generated blob for %d characters." % length
	safe_blob = textualize(blob)
	print "const char blob[%d] = " % (1 + length),
	for line in split_blob(safe_blob):
		text = "\n\t'%s'" % line
		print text,
	print ";\n\n",


if "__main__" == __name__:
	main()
