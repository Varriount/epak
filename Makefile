.PHONY: docs example python

CC = gcc
OBJ_DIR = obj
LIB_NAME = obj/libepak.a
WFLAGS = -Wall -W -Werror -Wno-unused -Wno-long-double
CFLAGS = -g
LFLAGS = -gA
CFLAGS += -fno-common -pipe

DESTDIR =
INSTALLDIR = $(DESTDIR)/usr/local
BINDIR = bin
LIBDIR = $(INSTALLDIR)/lib
INCDIR = $(INSTALLDIR)/include
INCDIR2 = $(INCDIR)/epak
TARGET_LIB = $(INSTALLDIR)/lib/libepak.a


default: all

$(TARGET_LIB): $(LIB_NAME)
	install -d $(INCDIR) $(INCDIR2) $(LIBDIR)
	install -c include/epak.h $(INCDIR)/epak.h
	install -c $(wildcard include/epak/*.h) $(INCDIR2)
	install -c $(LIB_NAME) $(LIBDIR)

all: python example docs
	rm -Rf build
	xcodebuild -configuration Debug -target "epak"
	#xcodebuild -configuration Release -target "Build ALL"

install: $(TARGET_LIB)
	@echo "Installed locally"

lib: $(LIB_NAME)
	@echo "Built library"

$(LIB_NAME): obj $(wildcard src/*.c)
	gcc -c $(CFLAGS) $(WFLAGS) -Iinclude -o obj/file.o src/file.c
	gcc -c $(CFLAGS) $(WFLAGS) -Iinclude -o obj/lzss.o src/lzss.c
	ar -r $(LIB_NAME) obj/file.o obj/lzss.o

obj:
	mkdir obj

example: $(LIB_NAME)
	gcc -o example/pretest example/test.c -Iinclude $(CFLAGS) $(WFLAGS) $(LIB_NAME)
	(cd example && ./pretest && cd ..)
	mv example/pretest example/test

python:
	./setup.py build
	echo "Python module built"

docs:
	doxygen

test:
	scan-build xcodebuild -configuration Debug -target "epak"

clean:
	rm -Rf docs obj build tags example/test example/pretest example/*.epak

ctags:
	~/bin/objctags -R src include
