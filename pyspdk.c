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
#include <python3.8/Python.h>
#include <python3.8/modsupport.h>
#include <python3.8/import.h>

#include <spdk/util.h>
#include <spdk/stdinc.h>
#include <spdk/nvme.h>
#include <spdk/vmd.h>
#include <spdk/env.h>

static PyObject* SpdkError = NULL;
static PyObject* SpdkEnvError = NULL;
static PyObject* SpdkProbeError = NULL;

static struct spdk_env_opts* g_spdk_env_opts = NULL;

struct NSEntry {
    struct spdk_nvme_ctrlr *ctrlr;
    struct spdk_nvme_ns *ns;
    TAILQ_ENTRY(NSEntry) link;
    struct spdk_nvme_qpair *qpair;
};

struct CtrlrEntry {
    struct spdk_nvme_ctrlr *ctrlr;
    TAILQ_ENTRY(CtrlrEntry) link;
    char name[1024];
};

struct ProbeCtx {
    TAILQ_HEAD(controllers_head, CtrlrEntry) controllers;
    TAILQ_HEAD(namespaces_head, NSEntry) namespaces;
};

static int
lazy_spdk_env_init() {
    int res;
    if (g_spdk_env_opts != NULL)
        return 0;
    g_spdk_env_opts = malloc(sizeof(struct spdk_env_opts));
    spdk_env_opts_init(g_spdk_env_opts);
    g_spdk_env_opts->name = "pyspdk";
    g_spdk_env_opts->shm_id = 0;
    res = spdk_env_init(g_spdk_env_opts);
    if (res < 0)
        free(g_spdk_env_opts);
    return res;
}

static bool
probe_cb(
    void* ctx,
    const struct spdk_nvme_transport_id* trid,
    struct spdk_nvme_ctrlr_opts* opts
) {
    return true;
}

static void
attach_cb(
    void* ctx,
    const struct spdk_nvme_transport_id *trid,
    struct spdk_nvme_ctrlr *ctrlr,
    const struct spdk_nvme_ctrlr_opts *opts
) {
    int nsid;
    int num_ns;
    struct CtrlrEntry* entry;
    struct NSEntry* ns_entry;
    struct spdk_nvme_ns* ns;
    const struct spdk_nvme_ctrlr_data* cdata;

    entry = malloc(sizeof(struct CtrlrEntry));
    if (entry == NULL) {
        // TODO: Handle errors in ProbeCtx and raise as exception when it
        // returns... Or, use async interface and report first error.
        return;
    }

    cdata = spdk_nvme_ctrlr_get_data(ctrlr);
    snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

    entry->ctrlr = ctrlr;
    TAILQ_INSERT_TAIL(&((struct ProbeCtx*)ctx)->controllers, entry, link);

    num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
    for (nsid = 1; nsid <= num_ns; nsid++) {
        ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
        if (ns == NULL)
            continue;
        if (!spdk_nvme_ns_is_active(ns))
            continue;
        ns_entry = malloc(sizeof(struct NSEntry));
        if (ns_entry == NULL) {
            // TODO: Handle errors in ProbeCtx and raise as exception when it
            // returns... Or, use async interface and report first error.
            continue;
        }
        ns_entry->ctrlr = ctrlr;
        ns_entry->ns = ns;
        TAILQ_INSERT_TAIL(&((struct ProbeCtx*)ctx)->namespaces, ns_entry, link);
    }
}

static PyObject*
probe(PyObject* self, PyObject* args) {
    struct ProbeCtx* ctx;
    PyObject* ret_list;

    struct NSEntry* ns_entry;
    struct NSEntry* tmp_ns_entry;
    struct CtrlrEntry* ctrlr_entry;
    struct CtrlrEntry* tmp_ctrlr_entry;

    if (lazy_spdk_env_init() < 0) {
        PyErr_SetString(SpdkEnvError, "Failed to initialize SPDK env");
        return NULL;
    }

    ctx = malloc(sizeof(struct ProbeCtx));
    if (ctx == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for SPDK probe");
        return NULL;
    }
    // TODO: How to detect errors in TAILQ_INIT?
    TAILQ_INIT(&ctx->controllers);
    TAILQ_INIT(&ctx->namespaces);

    if (spdk_nvme_probe(NULL, ctx, probe_cb, attach_cb, NULL)) {
        PyErr_SetString(SpdkProbeError, "Failed to probe for NVMe devices");
    }

    ret_list = PyList_New(0);
    if (ret_list != NULL) {
        TAILQ_FOREACH_SAFE(ctrlr_entry, &ctx->controllers, link, tmp_ctrlr_entry) {
            PyList_Append(ret_list, PyUnicode_FromString(ctrlr_entry->name));
        }
    }

    TAILQ_FOREACH_SAFE(ns_entry, &ctx->namespaces, link, tmp_ns_entry) {
        TAILQ_REMOVE(&ctx->namespaces, ns_entry, link);
        free(ns_entry);
    }
    TAILQ_FOREACH_SAFE(ctrlr_entry, &ctx->controllers, link, tmp_ctrlr_entry) {
        TAILQ_REMOVE(&ctx->controllers, ctrlr_entry, link);
        free(ctrlr_entry);
    }
    free(ctx);

    return ret_list;
}

static PyMethodDef
SPDKMethods[] = {
    {"probe", probe, METH_VARARGS, "Probe the system for SPDK devices."},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef
spdk_mod = {
    PyModuleDef_HEAD_INIT,
    "spdk",
    NULL,
    -1,
    SPDKMethods,
};

PyMODINIT_FUNC
PyInit_spdk(void) {
    PyObject *mod;

    // Create the module itself.
    mod = PyModule_Create(&spdk_mod);
    if (mod == NULL)
        goto error_cleanup;

    // Create the SpdkError.
    SpdkError = PyErr_NewException("spdk.SpdkError", NULL, NULL);
    if (SpdkError == NULL) 
        goto error_cleanup;
    Py_INCREF(SpdkError);
    if (PyModule_AddObject(mod, "SpdkError", SpdkError) < 0)
        goto error_cleanup;

    // Create the SpdkEnvError.
    SpdkEnvError = PyErr_NewException("spdk.SpdkEnvError", SpdkError, NULL);
    if (SpdkEnvError == NULL)
        goto error_cleanup;
    Py_INCREF(SpdkEnvError);
    if (PyModule_AddObject(mod, "SpdkEnvError", SpdkEnvError) < 0)
        goto error_cleanup;

    // Create the SpdkProbeError.
    SpdkProbeError = PyErr_NewException("spdk.SpdkProbeError", SpdkError, NULL);
    if (SpdkProbeError == NULL)
        goto error_cleanup;
    Py_INCREF(SpdkProbeError);
    if (PyModule_AddObject(mod, "SpdkProbeError", SpdkProbeError) < 0)
        goto error_cleanup;

    return mod;

// TODO: Is this canonically correct for C?
error_cleanup:
    if (mod != NULL) Py_DECREF(mod);
    if (SpdkError != NULL) Py_DECREF(SpdkError);
    if (SpdkEnvError != NULL) Py_DECREF(SpdkEnvError);
    if (SpdkProbeError != NULL) Py_DECREF(SpdkProbeError);
    return NULL;
}
