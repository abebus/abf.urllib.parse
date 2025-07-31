#define PY_SSIZE_T_CLEAN
#include "parse.h"
#include <Python.h>

// Helper: convert url_component_t to Python str
static inline PyObject *component_to_pystr(const url_component_t *comp) {
    return PyUnicode_FromStringAndSize(comp->start, (Py_ssize_t)comp->length);
}

// Helper: convert url_component_t to Python bytes
static inline PyObject *component_to_pybytes(const url_component_t *comp) {
    return PyBytes_FromStringAndSize(comp->start, (Py_ssize_t)comp->length);
}

// Global references to urllib.parse.ParseResult and ParseResultBytes types
static PyObject *parse_result_type = NULL;
static PyObject *parse_result_bytes_type = NULL;

// Helper: extract char* and length from str or bytes Python object
static inline int get_buffer_from_pyobject(PyObject *obj, char **buf,
                                           Py_ssize_t *len, const char *what) {
    if (PyBytes_Check(obj)) {
        *buf = PyBytes_AS_STRING(obj);
        *len = PyBytes_GET_SIZE(obj);
        return 1; // is_bytes
    } else if (PyUnicode_Check(obj)) {
        *buf = (char *)PyUnicode_AsUTF8AndSize(obj, len);
        if (!*buf)
            return -1;
        return 0; // is_str
    } else {
        PyErr_Format(PyExc_TypeError, "%s must be str or bytes", what);
        return -1;
    }
}

// abf_url_parse(url: str, scheme: str = '', allow_fragments: bool = True) ->
// ParseResult
static PyObject *abf_url_parse(PyObject *self, PyObject *args,
                               PyObject *kwargs) {
    PyObject *url_obj = NULL, *scheme_obj = NULL;
    char *url = NULL, *scheme = NULL;
    Py_ssize_t url_len = 0, scheme_len = 0;
    bool allow_fragments = true;
    static char *kwlist[] = {"url", "scheme", "allow_fragments", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|Op", kwlist, &url_obj,
                                     &scheme_obj, &allow_fragments)) {
        return NULL;
    }

    int is_bytes = get_buffer_from_pyobject(url_obj, &url, &url_len, "url");
    if (is_bytes < 0) {
        return NULL;
    }

    if (scheme_obj && scheme_obj != Py_None) {
        int scheme_is_bytes = get_buffer_from_pyobject(scheme_obj, &scheme,
                                                       &scheme_len, "scheme");
        if (scheme_is_bytes < 0) {
            return NULL;
        }
    } else {
        scheme = NULL;
        scheme_len = 0;
    }

    // Parse the URL
    // clang-format off
    url_parse_result_t result;
    int err;
    
    Py_BEGIN_ALLOW_THREADS 
    err = url_parse(url, (size_t)url_len, scheme,
                  allow_fragments, &result);
    Py_END_ALLOW_THREADS

    if (err != URL_PARSE_OK) {
        PyErr_SetString(PyExc_ValueError, "abf_url_parse: parse error");
        return NULL;
    }
    // clang-format on

    PyObject *args_tuple = PyTuple_New(6);
    if (!args_tuple) {
        return NULL;
    }

    PyObject *(*component_to_pyobj)(const url_component_t *);
    if (is_bytes) {
        component_to_pyobj = component_to_pybytes;
    } else {
        component_to_pyobj = component_to_pystr;
    }
    PyTuple_SET_ITEM(args_tuple, 0, component_to_pyobj(&result.scheme));
    PyTuple_SET_ITEM(args_tuple, 1, component_to_pyobj(&result.netloc));
    PyTuple_SET_ITEM(args_tuple, 2, component_to_pyobj(&result.path));
    PyTuple_SET_ITEM(args_tuple, 3, component_to_pyobj(&result.params));
    PyTuple_SET_ITEM(args_tuple, 4, component_to_pyobj(&result.query));
    PyTuple_SET_ITEM(args_tuple, 5, component_to_pyobj(&result.fragment));

    if (!args_tuple) {
        return NULL;
    }

    // Call ParseResult or ParseResultBytes constructor
    PyObject *result_type = parse_result_type;
    if (is_bytes) {
        result_type = parse_result_bytes_type;
    }
    PyObject *res = PyObject_CallObject(result_type, args_tuple);
    Py_DECREF(args_tuple);
    return res;
}

// abf_url_quote(s: str, safe: str = '/') -> str
static PyObject *abf_url_quote(PyObject *self, PyObject *args,
                               PyObject *kwargs) {
    // Parse arguments
    char *s = NULL;
    const char *safe = "/";
    Py_ssize_t s_len = 0, safe_len = 1;
    static char *kwlist[] = {"s", "safe", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|s#", kwlist, &s, &s_len,
                                     &safe, &safe_len))
        return NULL;

    // clang-format off
    int err;
    size_t bufsize = s_len * 3 + 1;
    char buf[bufsize];
    memcpy(buf, s, s_len);
    
    Py_BEGIN_ALLOW_THREADS
    err = url_quote(buf, s_len, safe, (size_t)safe_len);
    Py_END_ALLOW_THREADS

    if (err != URL_PARSE_OK) {
        PyErr_SetString(PyExc_ValueError, "abf_url_quote: quote error");
        return NULL;
    }
    // clang-format on
    PyObject *pyres = PyUnicode_FromString(buf);
    return pyres;
}

static PyMethodDef AbfParseMethods[] = {
    {"urlparse", (PyCFunction)abf_url_parse, METH_VARARGS | METH_KEYWORDS,
     "Faster urlparse(url, scheme='', allow_fragments=True) -> tuple"},
    {"quote", (PyCFunction)abf_url_quote, METH_VARARGS | METH_KEYWORDS,
     "Faster quote(s, safe='/') -> str"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef abfparsemodule = {
    PyModuleDef_HEAD_INIT, "abfparse", "A Bit Faster urllib.parse (C version)",
    -1, AbfParseMethods};

PyMODINIT_FUNC PyInit_abfparse(void) {
    PyObject *m = PyModule_Create(&abfparsemodule);

    if (!m) {
        return NULL;
    }

    // Import urllib.parse.ParseResult and ParseResultBytes
    PyObject *urllib_parse = PyImport_ImportModule("urllib.parse");
    if (!urllib_parse) {
        return NULL;
    }
    parse_result_type = PyObject_GetAttrString(urllib_parse, "ParseResult");
    parse_result_bytes_type =
        PyObject_GetAttrString(urllib_parse, "ParseResultBytes");

    // Ensure both types are loaded
    if (!parse_result_type || !parse_result_bytes_type) {
        PyErr_SetString(PyExc_RuntimeError,
                        "ParseResult or ParseResultBytes type not loaded");
        Py_XDECREF(parse_result_type);
        Py_XDECREF(parse_result_bytes_type);
        Py_DECREF(urllib_parse);
        Py_DECREF(m);
        return NULL;
    }

    Py_DECREF(urllib_parse);
    return m;
}
