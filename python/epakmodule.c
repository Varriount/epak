// vim:tabstop=4 shiftwidth=4 encoding=utf-8

#include "Python.h"
#include "epak.h"
#include <stdlib.h>
#include <string.h>

static const char *_utf8 = "utf8";

static PyObject *Epak_error = 0;

static PyObject *epak_set_password(PyObject* self, PyObject* args)
{
	const char *password;

	if (!PyArg_ParseTuple(args, "s", &password))
		return NULL;

	packfile_password(password);

	Py_INCREF(Py_None);
	return Py_None;
}

/////////////////////
// MODULE DEFINITION
/////////////////////

static PyMethodDef _epak_methods[] = {
	{"set_password",			epak_set_password, METH_VARARGS, 0},
	{ 0, 0, 0, 0}
};

PyMODINIT_FUNC initepak(void)
{
	PyObject *m, *d;

	m = Py_InitModule("epak", _epak_methods);
	d = PyModule_GetDict(m);
	Epak_error = PyErr_NewException("epak.Epak_error", NULL, NULL);
	PyDict_SetItemString(d, "Epak_error", Epak_error);
}
