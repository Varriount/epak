#!/usr/bin/env python
# -*- encoding: utf8 -*- vim: tabstop=4 shiftwidth=4 encoding=utf-8
"""Convenience wrapper around raw _epak C module."""

from __future__ import with_statement

import _epak


class Epak:
	def __init__(self, filename, mode):
		self.filename = filename
		self.mode = mode
		self.pak = _epak.open(filename, mode)
		assert self.pak

	def __del__(self):
		if self.pak:
			_epak.close(self.pak)
		self.pak = 0

	def __enter__(self):
		"""Returns itself to comply with the context manager protocol."""
		return self


	def __exit__(self, type, value, tb):
		"""f(?, ?, ?) -> False

		This always returns False, meaning that if an
		exception is raised inside the with block, it won't
		be caught. See the documentation for context managers
		for more information about this.

		Of course, this method exists to call close().
		"""
		self.close()
		return False


	def close(self):
		assert self.pak
		_epak.close(self.pak)
		self.pak = 0

	def seek(self, offset):
		assert self.pak
		ret = _epak.seek(self.pak, offset)
		assert 0 == ret

	def open_chunk(self, pack = 1):
		assert self.pak
		ret = _epak.open_chunk(self.pak, pack)
		assert ret
		self.pak = ret

	def close_chunk(self):
		assert self.pak
		ret = _epak.close_chunk(self.pak)
		assert ret
		self.pak = ret

	def getc(self):
		assert self.pak
		ret = _epak.getc(self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def putc(self, value):
		assert self.pak
		_epak.putc(self.pak, value)
		assert 0 == _epak.ferror(self.pak)

	def eof(self):
		assert self.pak
		return _epak.feof(self.pak)

	def igetw(self):
		assert self.pak
		ret = _epak.igetw(self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def igetl(self):
		assert self.pak
		ret = _epak.igetl(self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def iputw(self, value):
		assert self.pak
		_epak.iputw(self.pak, value)
		assert 0 == _epak.ferror(self.pak)

	def iputl(self, value):
		assert self.pak
		_epak.iputl(self.pak, value)
		assert 0 == _epak.ferror(self.pak)

	def mgetw(self):
		assert self.pak
		ret = _epak.mgetw(self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def mgetl(self):
		assert self.pak
		ret = _epak.mgetl(self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def mputw(self, value):
		assert self.pak
		_epak.mputw(self.pak, value)
		assert 0 == _epak.ferror(self.pak)

	def mputl(self, value):
		assert self.pak
		_epak.mputl(self.pak, value)
		assert 0 == _epak.ferror(self.pak)

	def read(self, length):
		assert self.pak
		ret = _epak.read(length, self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def write(self, data):
		assert self.pak
		ret = _epak.write(data, self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret

	def ungetc(self, value):
		assert self.pak
		ret = _epak.ungetc(value, self.pak)
		assert 0 == _epak.ferror(self.pak)
		return ret


def set_password(password):
	return _epak.set_password(password)


if __name__ == "__main__":
	main()
