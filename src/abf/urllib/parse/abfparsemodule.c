#define PY_SSIZE_T_CLEAN
#include "parse.h"
#include <Python.h>

// Helper: convert url_component_t to Python str or None
static PyObject *component_to_pystr(const url_component_t *comp) {
    if (!comp->start || comp->length == 0)
        PyUnicode_FromString("");
    return PyUnicode_FromStringAndSize(comp->start, comp->length);
}

// Global reference to urllib.parse.ParseResult type
static PyObject *parse_result_type = NULL;

// abf_url_parse(url: str, scheme: str = '', allow_fragments: bool = True) ->
// ParseResult
static PyObject *abf_url_parse(PyObject *self, PyObject *args,
                               PyObject *kwargs) {
    // Parse arguments
    const char *url = NULL, *scheme = NULL;
    Py_ssize_t url_len = 0, scheme_len = 0;
    bool allow_fragments = true;
    static char *kwlist[] = {"url", "scheme", "allow_fragments", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|s#p", kwlist, &url,
                                     &url_len, &scheme, &scheme_len,
                                     &allow_fragments)) {
        return NULL;
    }

    // Parse the URL
    // clang-format off
    url_parse_result_t result;
    int err;
    
    Py_BEGIN_ALLOW_THREADS 
    err = url_parse(url, (size_t)url_len, scheme, (size_t)scheme_len,
                  allow_fragments, &result);
    Py_END_ALLOW_THREADS

    if (err != URL_PARSE_OK) {
        PyErr_SetString(PyExc_ValueError, "abf_url_parse: parse error");
        return NULL;
    }
    // clang-format on

    // Build ParseResult tuple
    PyObject *args_tuple = PyTuple_New(6);
    if (!args_tuple) {
        return NULL;
    }

    PyTuple_SET_ITEM(args_tuple, 0, component_to_pystr(&result.scheme));
    PyTuple_SET_ITEM(args_tuple, 1, component_to_pystr(&result.netloc));
    PyTuple_SET_ITEM(args_tuple, 2, component_to_pystr(&result.path));
    PyTuple_SET_ITEM(args_tuple, 3, component_to_pystr(&result.params));
    PyTuple_SET_ITEM(args_tuple, 4, component_to_pystr(&result.query));
    PyTuple_SET_ITEM(args_tuple, 5, component_to_pystr(&result.fragment));

    if (!args_tuple)
        return NULL;

    // Call ParseResult constructor
    PyObject *res = PyObject_CallObject(parse_result_type, args_tuple);
    Py_DECREF(args_tuple);
    return res;
}

// abf_url_quote(s: str, safe: str = '/') -> str
static PyObject *abf_url_quote(PyObject *self, PyObject *args,
                               PyObject *kwargs) {
    // Parse arguments
    const char *s = NULL, *safe = "/";
    Py_ssize_t s_len = 0, safe_len = 1;
    static char *kwlist[] = {"s", "safe", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|s#", kwlist, &s, &s_len,
                                     &safe, &safe_len))
        return NULL;

    // Allocate buffer for quoted string
    size_t max_quoted = (size_t)s_len * 3 + 1;
    char stack_buf[4096];
    char *quoted =
        (max_quoted <= sizeof(stack_buf)) ? stack_buf : malloc(max_quoted);
    if (!quoted) {
        PyErr_NoMemory();
        return NULL;
    }

    // Perform quoting
    // clang-format off
    size_t quoted_len = 0;
    int err;

    Py_BEGIN_ALLOW_THREADS
    err = url_quote(s, (size_t)s_len, safe, (size_t)safe_len, quoted, max_quoted,
                  &quoted_len);
    Py_END_ALLOW_THREADS

    if (err != URL_PARSE_OK) {
        if (quoted != stack_buf) {
            free(quoted);
        }
        PyErr_SetString(PyExc_ValueError, "abf_url_quote: quote error");
        return NULL;
    }
    // clang-format on

    // Build Python string result
    PyObject *pyres = PyUnicode_FromStringAndSize(quoted, quoted_len);
    if (quoted != stack_buf)
        free(quoted);
    return pyres;
}

static PyMethodDef AbfParseMethods[] = {
    {"url_parse", (PyCFunction)abf_url_parse, METH_VARARGS | METH_KEYWORDS,
     "Fast url_parse(url, scheme='', allow_fragments=True) -> dict"},
    {"url_quote", (PyCFunction)abf_url_quote, METH_VARARGS | METH_KEYWORDS,
     "Fast url_quote(s, safe='/') -> str"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef abfparsemodule = {
    PyModuleDef_HEAD_INIT, "abfparse", "A Bit Faster urllib.parse (C version)",
    -1, AbfParseMethods};

PyMODINIT_FUNC PyInit_abfparse(void) {
    PyObject *m = PyModule_Create(&abfparsemodule);

    if (!m) {
        return NULL;
    }

    // Import urllib.parse.ParseResult
    PyObject *urllib_parse = PyImport_ImportModule("urllib.parse");
    if (!urllib_parse) {
        return NULL;
    }
    parse_result_type = PyObject_GetAttrString(urllib_parse, "ParseResult");

    // Ensure ParseResult type is loaded
    if (!parse_result_type) {
        PyErr_SetString(PyExc_RuntimeError, "ParseResult type not loaded");
        return NULL;
    }

    Py_DECREF(urllib_parse);

    if (!parse_result_type) {
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
