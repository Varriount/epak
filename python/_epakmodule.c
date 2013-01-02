#include "Python.h"
#include "epak.h"
#include <stdlib.h>
#include <string.h>

static const char *_utf8 = "utf8";

static PyObject *Epak_error = 0;


#define RETURN_NONE() do { Py_INCREF(Py_None); return Py_None;} while(0)
#define RAISE(X) do { PyErr_SetString(Epak_error, X); return NULL;} while(0)

#if 0
#define DLOG(X, ...)		printf("epak.C:" X, ##__VA_ARGS__)
#else
#define DLOG(X, ...)		;
#endif



static PyObject *epak_set_password(PyObject* self, PyObject* args)
{
	const char *password;
	if (!PyArg_ParseTuple(args, "s", &password)) return NULL;

	packfile_password(password);

	RETURN_NONE();
}

static PyObject *epak_open(PyObject *self, PyObject *args)
{
	DLOG("Opening pack\n");
	const char *filename, *mode;
	if (!PyArg_ParseTuple(args, "ss", &filename, &mode)) return NULL;

	PACKFILE *p = pack_fopen(filename, mode);
	if (!p) RAISE("Error opening packfile");
	return Py_BuildValue("i", p);
}

static PyObject *epak_close(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	if (!PyArg_ParseTuple(args, "i", &p)) return NULL;

	DLOG("Closing packfile %p\n", p);
	const int ret = pack_fclose(p);
	if (ret) RAISE("Error closing packfile");
	RETURN_NONE();
}

static PyObject *epak_seek(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	int offset;
	if (!PyArg_ParseTuple(args, "ii", &p, &offset)) return NULL;

	const int ret = pack_fseek(p, offset);
	if (ret) RAISE("Error seeking packfile");
	RETURN_NONE();
}

static PyObject *epak_open_chunk(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	int pack;
	if (!PyArg_ParseTuple(args, "ii", &p, &pack)) return NULL;

	DLOG("Opening chunk inside %p\n", p);
	PACKFILE *valid = pack_fopen_chunk(p, pack);
	if (!valid) RAISE("Error opening chunk");
	return Py_BuildValue("i", valid);
}

static PyObject *epak_close_chunk(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	if (!PyArg_ParseTuple(args, "i", &p)) return NULL;

	PACKFILE *valid = pack_fclose_chunk(p);
	if (!valid) RAISE("Error closing chunk");
	DLOG("Closed chunk, parent %p\n", valid);
	return Py_BuildValue("i", valid);
}

static PyObject *epak_write(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	int len;
	const char *data;
	if (!PyArg_ParseTuple(args, "s#i", &data, &len, &p)) return NULL;

	const long ret = pack_fwrite(data, len, p);
	if (ret != len) RAISE("Couldn't write enough data");
	return Py_BuildValue("l", ret);
}

static PyObject *epak_skip_chunks(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	unsigned int to_skip;
	if (!PyArg_ParseTuple(args, "iI", &p, &to_skip)) return NULL;

	if (pack_skip_chunks(p, to_skip))
		RAISE("Errors detected skipping chunks");
	RETURN_NONE();
}

static PyObject *epak_read(PyObject *self, PyObject *args)
{
	PACKFILE *p;
	long len;
	char *data;
	if (!PyArg_ParseTuple(args, "li", &len, &p)) return NULL;

	data = malloc(len);
	if (!data) RAISE("Couldn't allocate enough memory for reading data!");
	const long ret = pack_fread(data, len, p);
	if (ret != len)
		printf("epak.low_level_warning: pack_fread() returning less data!\n");

	PyObject *o = Py_BuildValue("s#", data, ret);
	free(data);
	return o;
}

// To avoid typing, repeat through a macro a typical (int)(PACKFILE *) func.
#define MAKE_GET_INT(NAME1,NAME2) \
static PyObject *NAME1(PyObject *self, PyObject *args) {	\
	PACKFILE *p;											\
	if (!PyArg_ParseTuple(args, "i", &p)) return NULL;		\
	return Py_BuildValue("i", NAME2(p));				\
}

// Same for longs.
#define MAKE_GET_LONG(NAME1,NAME2) \
static PyObject *NAME1(PyObject *self, PyObject *args) {	\
	PACKFILE *p;											\
	if (!PyArg_ParseTuple(args, "i", &p)) return NULL;		\
	return Py_BuildValue("l", NAME2(p));				\
}

#define MAKE_PUT_INT(NAME1,NAME2) \
static PyObject *NAME1(PyObject *self, PyObject *args) {		\
	PACKFILE *p; int val;										\
	if (!PyArg_ParseTuple(args, "ii", &val, &p)) return NULL;	\
	return Py_BuildValue("i", NAME2(val, p));					\
}

#define MAKE_PUT_LONG(NAME1,NAME2) \
static PyObject *NAME1(PyObject *self, PyObject *args) {		\
	PACKFILE *p; int val;										\
	if (!PyArg_ParseTuple(args, "li", &val, &p)) return NULL;	\
	return Py_BuildValue("l", NAME2(val, p));					\
}


MAKE_GET_INT(epak_getc, pack_getc)
MAKE_PUT_INT(epak_putc, pack_putc)
MAKE_GET_INT(epak_feof, pack_feof)
MAKE_GET_INT(epak_ferror, pack_ferror)
MAKE_GET_INT(epak_igetw, pack_igetw)
MAKE_GET_LONG(epak_igetl, pack_igetl)
MAKE_PUT_INT(epak_iputw, pack_iputw)
MAKE_PUT_LONG(epak_iputl, pack_iputl)
MAKE_GET_INT(epak_mgetw, pack_mgetw)
MAKE_GET_LONG(epak_mgetl, pack_mgetl)
MAKE_PUT_INT(epak_mputw, pack_mputw)
MAKE_PUT_LONG(epak_mputl, pack_mputl)
MAKE_PUT_INT(epak_ungetc, pack_ungetc)

/////////////////////
// MODULE DEFINITION
/////////////////////

static PyMethodDef _epak_methods[] = {
	{"set_password",			epak_set_password, METH_VARARGS, 0},
	{"open",					epak_open, METH_VARARGS, 0},
	{"close",					epak_close, METH_VARARGS, 0},
	{"seek",					epak_seek, METH_VARARGS, 0},
	{"open_chunk",				epak_open_chunk, METH_VARARGS, 0},
	{"close_chunk",				epak_close_chunk, METH_VARARGS, 0},
	{"getc",					epak_getc, METH_VARARGS, 0},
	{"putc",					epak_putc, METH_VARARGS, 0},
	{"feof",					epak_feof, METH_VARARGS, 0},
	{"ferror",					epak_ferror, METH_VARARGS, 0},
	{"igetw",					epak_igetw, METH_VARARGS, 0},
	{"igetl",					epak_igetl, METH_VARARGS, 0},
	{"iputl",					epak_iputl, METH_VARARGS, 0},
	{"mgetw",					epak_mgetw, METH_VARARGS, 0},
	{"mgetw",					epak_mgetw, METH_VARARGS, 0},
	{"mputw",					epak_mputw, METH_VARARGS, 0},
	{"mputl",					epak_mputl, METH_VARARGS, 0},
	{"write",					epak_write, METH_VARARGS, 0},
	{"read",					epak_read, METH_VARARGS, 0},
	{"ungetc",					epak_ungetc, METH_VARARGS, 0},
	{"skip_chunks",				epak_skip_chunks, METH_VARARGS, 0},
	{ 0, 0, 0, 0}
};

PyMODINIT_FUNC init_epak(void)
{
	PyObject *m, *d;

	m = Py_InitModule("_epak", _epak_methods);
	d = PyModule_GetDict(m);
	Epak_error = PyErr_NewException("_epak.Epak_error", NULL, NULL);
	PyDict_SetItemString(d, "Epak_error", Epak_error);
}

// vim:tabstop=4 shiftwidth=4
