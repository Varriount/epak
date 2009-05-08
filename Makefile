.PHONY: docs example

default: all

all: example docs
	rm -Rf build
	xcodebuild -configuration Debug -target "epak"
	#xcodebuild -configuration Release -target "Build ALL"

example:
	gcc -o example/pretest example/test.c src/*.c -Iinclude -W -Wall -Wunused -Werror
	(cd example && ./pretest && cd ..)
	mv example/pretest example/test

docs:
	doxygen

test:
	scan-build xcodebuild -configuration Debug -target "epak"

clean:
	rm -Rf build example/test example/pretest example/*.epak

ctags:
	~/bin/objctags -R src include
