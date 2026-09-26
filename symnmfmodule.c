#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "symnmf.h"

/*
 * symnmfmodule.c - Python C extension "symnmfmodule".
 * Wraps the C implementation (symnmf.c) so Python can call:
 *   sym(X), ddg(X), norm(X)  -> n x n matrix as a list of lists of floats
 *   symnmf(H, W)             -> final H (n x k) as a list of lists of floats
 */

/* Signature shared by sym / ddg / norm in symnmf.h. */
typedef double **(*goal_func)(double **, int, int);

/*
 * Converts a Python list of equal-length lists of numbers into a new C
 * matrix and stores its dimensions in *rows and *cols.
 * Returns NULL with a Python exception set on bad input or memory failure.
 */
static double **py_to_matrix(PyObject *obj, int *rows, int *cols)
{
    double **m;
    PyObject *row;
    Py_ssize_t i, j, r, c;

    if (!PyList_Check(obj) || (r = PyList_Size(obj)) == 0
        || !PyList_Check(PyList_GetItem(obj, 0))
        || (c = PyList_Size(PyList_GetItem(obj, 0))) == 0) {
        PyErr_SetString(PyExc_ValueError, "expected a non-empty list of lists");
        return NULL;
    }
    m = alloc_matrix((int)r, (int)c);
    if (m == NULL) {
        return (double **)PyErr_NoMemory();
    }
    for (i = 0; i < r; i++) {
        row = PyList_GetItem(obj, i);
        if (!PyList_Check(row) || PyList_Size(row) != c) {
            free_matrix(m);
            PyErr_SetString(PyExc_ValueError, "rows must have equal length");
            return NULL;
        }
        for (j = 0; j < c; j++) {
            m[i][j] = PyFloat_AsDouble(PyList_GetItem(row, j));
        }
    }
    if (PyErr_Occurred()) {
        free_matrix(m);
        return NULL;
    }
    *rows = (int)r;
    *cols = (int)c;
    return m;
}

/*
 * Converts a rows x cols C matrix into a new Python list of lists of floats.
 * Returns NULL with a Python exception set on failure.
 */
static PyObject *matrix_to_py(double **m, int rows, int cols)
{
    PyObject *result, *row, *val;
    int i, j;

    result = PyList_New(rows);
    if (result == NULL) {
        return NULL;
    }
    for (i = 0; i < rows; i++) {
        row = PyList_New(cols);
        if (row == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, row); /* steals the reference to row */
        for (j = 0; j < cols; j++) {
            val = PyFloat_FromDouble(m[i][j]);
            if (val == NULL) {
                Py_DECREF(result);
                return NULL;
            }
            PyList_SET_ITEM(row, j, val); /* steals the reference to val */
        }
    }
    return result;
}

/*
 * Shared body of sym/ddg/norm: parses the single argument X (n x d list of
 * lists), runs func on it and returns the n x n result as a list of lists.
 */
static PyObject *run_goal(PyObject *args, goal_func func)
{
    PyObject *x_obj, *result;
    double **X, **R;
    int n, d;

    if (!PyArg_ParseTuple(args, "O", &x_obj)) {
        return NULL;
    }
    X = py_to_matrix(x_obj, &n, &d);
    if (X == NULL) {
        return NULL;
    }
    R = func(X, n, d);
    free_matrix(X);
    if (R == NULL) {
        return PyErr_NoMemory();
    }
    result = matrix_to_py(R, n, n);
    free_matrix(R);
    return result;
}

/* Python: sym(X) -> similarity matrix A. */
static PyObject *py_sym(PyObject *self, PyObject *args)
{
    (void)self; /* self is unused; cast to void to silence the warning */
    return run_goal(args, sym);
}

/* Python: ddg(X) -> diagonal degree matrix D. */
static PyObject *py_ddg(PyObject *self, PyObject *args)
{
    (void)self; /* self is unused; cast to void to silence the warning */
    return run_goal(args, ddg);
}

/* Python: norm(X) -> normalized similarity matrix W. */
static PyObject *py_norm(PyObject *self, PyObject *args)
{
    (void)self; /* self is unused; cast to void to silence the warning */
    return run_goal(args, norm);
}

/*
 * Python: symnmf(H, W) -> final H.
 * H is the initial n x k matrix and W the n x n normalized similarity matrix.
 */
static PyObject *py_symnmf(PyObject *self, PyObject *args)
{
    PyObject *h_obj, *w_obj, *result = NULL;
    double **H0, **W = NULL, **H;
    int n, k, wr, wc;

    (void)self; /* self is unused; cast to void to silence the warning */
    if (!PyArg_ParseTuple(args, "OO", &h_obj, &w_obj)) {
        return NULL;
    }
    H0 = py_to_matrix(h_obj, &n, &k);
    if (H0 != NULL) {
        W = py_to_matrix(w_obj, &wr, &wc);
    }
    if (W != NULL && (wr != n || wc != n)) {
        PyErr_SetString(PyExc_ValueError, "W must be n x n where H is n x k");
    } else if (W != NULL) {
        H = symnmf(H0, W, n, k);
        result = (H == NULL) ? PyErr_NoMemory() : matrix_to_py(H, n, k);
        free_matrix(H);
    }
    free_matrix(H0);
    free_matrix(W);
    return result;
}

static PyMethodDef symnmf_methods[] = {
    {"sym", (PyCFunction)py_sym, METH_VARARGS,
     PyDoc_STR("sym(X) -> similarity matrix A of the points X (list of lists)")},
    {"ddg", (PyCFunction)py_ddg, METH_VARARGS,
     PyDoc_STR("ddg(X) -> diagonal degree matrix D of the points X")},
    {"norm", (PyCFunction)py_norm, METH_VARARGS,
     PyDoc_STR("norm(X) -> normalized similarity matrix W of the points X")},
    {"symnmf", (PyCFunction)py_symnmf, METH_VARARGS,
     PyDoc_STR("symnmf(H, W) -> final H after SymNMF updates from initial H")},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef symnmf_module = {
    PyModuleDef_HEAD_INIT,
    "symnmfmodule",
    "SymNMF clustering implemented in C.",
    -1,
    symnmf_methods,
    NULL, NULL, NULL, NULL
};

/* Module initialization function called by Python on import. */
PyMODINIT_FUNC PyInit_symnmfmodule(void)
{
    return PyModule_Create(&symnmf_module);
}
