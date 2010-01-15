#!/usr/bin/env python
# -*- encoding: utf8 -*- vim: tabstop=4 shiftwidth=4 encoding=utf-8

import os
import string
import sys
from distutils.core import setup, Extension

include_dirs = ["include"]
lib_dirs = ["obj"]
libraries = ["epak"]
extra_compile_args = ["-Wall", "-W", "-Werror",  "-Wno-unused",
	"-g",  "-fno-common",  "-pipe"]
extra_link_args = ["-gA"]


def main():
	setup(name='epak',
		author="Grzegorz Adam Hankiewicz",
		author_email="gradha@titanium.sabren.com",
		description="Python bindings to epak library",
		package_dir = {'': 'python'},
		py_modules=['epak'],
		ext_modules=[Extension('_epak', ['python/_epakmodule.c'],
			include_dirs=include_dirs,
			library_dirs=lib_dirs,
			libraries=libraries,
			extra_compile_args=extra_compile_args,
			extra_link_args=extra_link_args
			)],
		url='http://elhaso.com/',
		license='MIT'
		)

if __name__ == "__main__":
	main()
