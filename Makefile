.PHONY: docs

default: all

all: #docs
	rm -Rf build
	xcodebuild -configuration Debug -target "epak"
	#xcodebuild -configuration Release -target "Build ALL"

docs:
	doxygen

test:
	scan-build xcodebuild -configuration Debug -target "epak"

clean:
	rm -Rf build

ctags:
	~/bin/objctags -R src include
