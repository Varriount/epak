.PHONY: docs example python

PYTHON = python
CC = llvm-gcc
OBJ_DIR = obj
LIB_NAME = obj/libepak.a
WFLAGS = -Wall -W -Werror -Wno-unused
CFLAGS = -g -DNDEBUG
LFLAGS = -gA
CFLAGS += -fno-common -pipe

DESTDIR =
INSTALLDIR = $(DESTDIR)/usr/local
BINDIR = bin
LIBDIR = $(INSTALLDIR)/lib
INCDIR = $(INSTALLDIR)/include
INCDIR2 = $(INCDIR)/epak
TARGET_LIB = $(INSTALLDIR)/lib/libepak.a
LIB_DEBUG = build/libepak-Debug.a
LIB_RELEASE = build/libepak-Release.a


default: all

$(TARGET_LIB): $(LIB_NAME)
	install -d $(INCDIR) $(INCDIR2) $(LIBDIR)
	install -c include/epak.h $(INCDIR)/epak.h
	install -c $(wildcard include/epak/*.h) $(INCDIR2)
	install -c $(LIB_NAME) $(LIBDIR)

#all: $(LIB_DEBUG) $(LIB_RELEASE) python example docs
all: $(LIB_DEBUG) $(LIB_RELEASE) example docs
	@echo "Now that the code is built, run make install."

$(LIB_DEBUG): $(wildcard src/*.c) $(wildcard include/*.h) $(wildcard include/epak/*h)
	xcodebuild -scheme epak-debug

$(LIB_RELEASE): $(wildcard src/*.c) $(wildcard include/*.h) $(wildcard include/epak/*h)
	xcodebuild -scheme epak-release

#install: $(TARGET_LIB) install_python
install: $(TARGET_LIB)
	@echo "Installed locally"

lib: $(LIB_NAME)
	@echo "Built library"

$(LIB_NAME): obj $(wildcard src/*.c)
	$(CC) -c $(CFLAGS) $(WFLAGS) -Iinclude -o obj/file.o src/file.c
	$(CC) -c $(CFLAGS) $(WFLAGS) -Iinclude -o obj/lzss.o src/lzss.c
	ar -r $(LIB_NAME) obj/file.o obj/lzss.o

obj:
	mkdir obj

example: $(LIB_NAME)
	$(CC) -o example/pretest example/test.c -Iinclude $(CFLAGS) $(WFLAGS) $(LIB_NAME)
	(cd example && ./pretest && cd ..)
	mv example/pretest example/test

python_test: example
	(cd example && $(PYTHON) ./test.py)
	@echo "Python example runs finished"

python:
	$(PYTHON) ./setup.py build
	echo "Python module built"

install_python: python
	$(PYTHON) ./setup.py install
	@echo "Now that epak module is available, try running make python_test"

docs:
	doxygen

test:
	scan-build xcodebuild -configuration Debug -target "epak"

clean:
	rm -Rf docs obj build tags example/test example/pretest example/*.epak

ctags:
	~/bin/objctags -R src include

gen_nimrod_c:
	echo "// File generated, see gen_nimrod_c in Makefile" > nimrod/bundled.h
	grep -hv "#include" include/epak/*.h >> nimrod/bundled.h
	cp nimrod/bundled_start.txt nimrod/bundled.c
	grep -hv "#include" src/file.c >> nimrod/bundled.c
	grep -hv "#include" src/lzss.c >> nimrod/bundled.c
