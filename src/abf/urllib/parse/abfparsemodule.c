#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "parse.h"

// Helper: convert url_component_t to Python str
static PyObject *component_to_pystr(const url_component_t *comp) {
    if (!comp->start || !comp->length)
        Py_RETURN_NONE;
    return PyUnicode_FromStringAndSize(comp->start, comp->length);
}

// Global reference to urllib.parse.ParseResult type
static PyObject *parse_result_type = NULL;

// abf_url_parse(url: str, scheme: str = '', allow_fragments: bool = True) -> ParseResult
static PyObject *abf_url_parse(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *url = NULL, *scheme = NULL;
    Py_ssize_t url_len = 0, scheme_len = 0;
    int allow_fragments = 1;
    static char *kwlist[] = {"url", "scheme", "allow_fragments", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|s#p", kwlist,
                                     &url, &url_len, &scheme, &scheme_len, &allow_fragments))
        return NULL;
    url_parse_result_t result;
    int err = url_parse(url, (size_t)url_len, scheme, (size_t)scheme_len, allow_fragments, &result);
    if (err != URL_PARSE_OK) {
        PyErr_SetString(PyExc_ValueError, "abf_url_parse: parse error");
        return NULL;
    }
    if (!parse_result_type) {
        PyErr_SetString(PyExc_RuntimeError, "ParseResult type not loaded");
        return NULL;
    }
    PyObject *args_tuple = Py_BuildValue(
        "(OOOOOO)",
        component_to_pystr(&result.scheme),
        component_to_pystr(&result.netloc),
        component_to_pystr(&result.path),
        component_to_pystr(&result.params),
        component_to_pystr(&result.query),
        component_to_pystr(&result.fragment)
    );
    if (!args_tuple) return NULL;
    PyObject *res = PyObject_CallObject(parse_result_type, args_tuple);
    Py_DECREF(args_tuple);
    return res;
}

// abf_url_quote(s: str, safe: str = '/') -> str
static PyObject *abf_url_quote(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *s = NULL, *safe = "/";
    Py_ssize_t s_len = 0, safe_len = 1;
    static char *kwlist[] = {"s", "safe", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|s#", kwlist, &s, &s_len, &safe, &safe_len))
        return NULL;
    quote_result_t result;
    int err = url_quote(s, (size_t)s_len, safe, (size_t)safe_len, &result);
    if (err != URL_PARSE_OK) {
        PyErr_SetString(PyExc_ValueError, "abf_url_quote: quote error");
        return NULL;
    }
    PyObject *pyres = PyUnicode_FromStringAndSize(result.data, result.length);
    quote_result_free(&result);
    return pyres;
}

static PyMethodDef AbfParseMethods[] = {
    {"url_parse", (PyCFunction)abf_url_parse, METH_VARARGS | METH_KEYWORDS, "Fast url_parse(url, scheme='', allow_fragments=True) -> dict"},
    {"url_quote", (PyCFunction)abf_url_quote, METH_VARARGS | METH_KEYWORDS, "Fast url_quote(s, safe='/') -> str"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef abfparsemodule = {
    PyModuleDef_HEAD_INIT,
    "abfparse",
    "A Bit Faster urllib.parse (C version)",
    -1,
    AbfParseMethods
};

PyMODINIT_FUNC PyInit_abfparse(void) {
    PyObject *m = PyModule_Create(&abfparsemodule);
    if (!m) return NULL;
    // Import urllib.parse.ParseResult
    PyObject *urllib_parse = PyImport_ImportModule("urllib.parse");
    if (!urllib_parse) return NULL;
    parse_result_type = PyObject_GetAttrString(urllib_parse, "ParseResult");
    Py_DECREF(urllib_parse);
    if (!parse_result_type) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
