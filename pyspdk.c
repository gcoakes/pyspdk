/*
 * Copyright (c) 2021 Oakes, Gregory C. <gregcoakes@gmail.com>
 * Author: Oakes, Gregory C. <gregcoakes@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <spdk/stdinc.h>
#include <spdk/nvme.h>
#include <spdk/vmd.h>
#include <spdk/env.h>

static PyObject*
scan(PyObject* self, PyObject* args) {
    return PyUnicode_FromString("Hello, world!");
}

static PyMethodDef
SPDKMethods[] = {
    {"scan", scan, METH_VARARGS, "Scan the system for SPDK devices."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef spdk_mod = {
    PyModuleDef_HEAD_INIT,
    "spdk",
    NULL,
    -1,
    SPDKMethods,
};

PyMODINIT_FUNC
PyInit_spdk(void)
{
    PyObject *m;

    m = PyModule_Create(&spdk_mod);
    if (m == NULL)
        return NULL;

    return m;
}
