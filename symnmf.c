/*
 * symnmf.c - SymNMF matrix computations and a standalone command line program.
 *
 * goal is one of: sym, ddg, norm. The resulting n x n matrix is printed
 * with 4 decimal places, comma separated, one row per line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "symnmf.h"

#define ERROR_MSG "An Error Has Occurred\n"

/* ---------------------------------------------------------------------- */
/* Matrix memory                                                          */
/* ---------------------------------------------------------------------- */

/*
 * Allocates a rows x cols matrix of zeros: one data block holding all the
 * entries, plus an array of row pointers into that block.
 * Returns NULL if any allocation fails.
 */
double **alloc_matrix(int rows, int cols)
{
    double *data;
    double **m;
    int i;

    data = calloc((size_t)rows * (size_t)cols, sizeof(double)); /* contiguous data block */
    if (data == NULL) {
        return NULL;
    }
    m = malloc((size_t)rows * sizeof(double *));
    if (m == NULL) {
        free(data);
        return NULL;
    }
    for (i = 0; i < rows; i++) {
        m[i] = data + (size_t)i * (size_t)cols;
    }
    return m;
}

/* Frees a matrix created by alloc_matrix (data block, then row array). */
void free_matrix(double **m)
{
    if (m == NULL) {
        return;
    }
    free(m[0]); /* free the contiguous data block m[0] is pointing to */
    free(m);
}

/* Returns a newly allocated copy of the rows x cols matrix src, or NULL. */
static double **copy_matrix(double **src, int rows, int cols)
{
    double **dst;

    dst = alloc_matrix(rows, cols);
    if (dst != NULL) {
        memcpy(dst[0], src[0], (size_t)rows * (size_t)cols * sizeof(double));
    }
    return dst;
}

/* ---------------------------------------------------------------------- */
/* sym / ddg / norm                                                       */
/* ---------------------------------------------------------------------- */

/* Squared Euclidean distance between the d-dimensional points p and q. */
static double squared_distance(const double *p, const double *q, int d)
{
    double sum = 0.0, diff;
    int i;

    for (i = 0; i < d; i++) {
        diff = p[i] - q[i];
        sum += diff * diff;
    }
    return sum;
}

/*
 * Similarity matrix: a_ij = exp(-||x_i - x_j||^2 / 2) for i != j and
 * a_ii = 0. The matrix is symmetric, so each pair is computed once.
 */
double **sym(double **X, int n, int d)
{
    double **A;
    int i, j;

    A = alloc_matrix(n, n);
    if (A == NULL) {
        return NULL;
    }
    /* diagonal elements are zero by using calloc in alloc_matrix */
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            A[i][j] = exp(-squared_distance(X[i], X[j], d) / 2.0);
            A[j][i] = A[i][j];
        }
    }
    return A;
}

/* Degree of each vertex: deg[i] = sum of row i of A. Returns NULL on failure. */
static double *degrees(double **A, int n)
{
    double *deg;
    int i, j;

    deg = calloc((size_t)n, sizeof(double));
    if (deg == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            deg[i] += A[i][j];
        }
    }
    return deg;
}

/* Diagonal degree matrix built from a similarity matrix A (n x n). */
static double **ddg_from_sym(double **A, int n)
{
    double **D;
    double *deg;
    int i;

    deg = degrees(A, n);
    D = alloc_matrix(n, n);
    if (deg == NULL || D == NULL) {
        free(deg);
        free_matrix(D);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        D[i][i] = deg[i];
    }
    free(deg);

    return D;
}

/*
 * Normalized similarity from A (n x n): w_ij = a_ij / sqrt(d_i * d_j), which
 * equals D^-1/2 A D^-1/2 without building D or doing matrix products.
 */
static double **norm_from_sym(double **A, int n)
{
    double **W;
    double *deg;
    int i, j;

    deg = degrees(A, n);
    W = alloc_matrix(n, n);
    if (deg == NULL || W == NULL) {
        free(deg);
        free_matrix(W);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            W[i][j] = A[i][j] / sqrt(deg[i] * deg[j]);
        }
    }
    free(deg);
    return W;
}

/* Diagonal degree matrix D (n x n) of the points X (n x d). */
double **ddg(double **X, int n, int d)
{
    double **A, **D;

    A = sym(X, n, d);
    if (A == NULL) {
        return NULL;
    }
    D = ddg_from_sym(A, n);
    free_matrix(A);
    return D;
}

/* Normalized similarity matrix W (n x n) of the points X (n x d). */
double **norm(double **X, int n, int d)
{
    double **A, **W;

    A = sym(X, n, d);
    if (A == NULL) {
        return NULL;
    }
    W = norm_from_sym(A, n);
    free_matrix(A);
    return W;
}

/* ---------------------------------------------------------------------- */
/* symnmf                                                                 */
/* ---------------------------------------------------------------------- */

/* C (n x p) = A (n x m) * B (m x p). C must be a different matrix from A and B. */
static void mat_mul(double **A, double **B, double **C, int n, int m, int p)
{
    int i, j, t;
    double a;

    for (i = 0; i < n; i++) {
        for (j = 0; j < p; j++) {
            C[i][j] = 0.0;
        }
        for (t = 0; t < m; t++) { /* less cache misses iterating over rows */
            a = A[i][t];
            for (j = 0; j < p; j++) {
                C[i][j] += a * B[t][j];
            }
        }
    }
}

/* G (k x k) = H^T H, for H of size n x k. */
static void h_trans_h(double **H, double **G, int n, int k)
{
    int i, a, b;

    for (a = 0; a < k; a++) {
        for (b = 0; b < k; b++) {
            G[a][b] = 0.0;
        }
    }
    for (i = 0; i < n; i++) {
        for (a = 0; a < k; a++) {
            for (b = 0; b < k; b++) {
                G[a][b] += H[i][a] * H[i][b];
            }
        }
    }
}

/*
 * One SymNMF update: next_ij = H_ij * (1 - BETA + BETA * (WH)_ij / (HH^TH)_ij).
 * H H^T H is computed as H * (H^T H) (O(n k^2)) instead of (H H^T) H (O(n^2 k)).
 * DENOM_EPS is always added to the denominator to avoid division by zero.
 * ws holds three temporary matrices that symnmf allocates once and this
 * function reuses on every iteration.
 */
static void update_step(double **H, double **W, double **next,
                        double **ws[3], int n, int k)
{
    double **WH = ws[0], **HtH = ws[1], **HHtH = ws[2];
    int i, j;

    mat_mul(W, H, WH, n, n, k);
    h_trans_h(H, HtH, n, k);
    mat_mul(H, HtH, HHtH, n, k, k);
    for (i = 0; i < n; i++) {
        for (j = 0; j < k; j++) {
            next[i][j] = H[i][j] * (1.0 - BETA + BETA * WH[i][j]
                                    / (HHtH[i][j] + DENOM_EPS));
        }
    }
}

/* Squared Frobenius norm of (A - B), both n x k. */
static double frob_diff_sq(double **A, double **B, int n, int k)
{
    double sum = 0.0, diff;
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < k; j++) {
            diff = A[i][j] - B[i][j];
            sum += diff * diff;
        }
    }
    return sum;
}

/*
 * SymNMF optimization. Iterates update_step until ||H(t+1) - H(t)||_F^2 <
 * EPSILON or MAX_ITER iterations were done, and returns the last H.
 * Two H buffers are swapped each iteration so no per-iteration allocation
 * is needed. Returns NULL on allocation failure (everything is freed).
 */
double **symnmf(double **H0, double **W, int n, int k)
{
    double **H, **next, **tmp, **ws[3];
    int iter;
    double diff;

    H = copy_matrix(H0, n, k);
    next = alloc_matrix(n, k);
    ws[0] = alloc_matrix(n, k); /* WH */
    ws[1] = alloc_matrix(k, k); /* H^T H */
    ws[2] = alloc_matrix(n, k); /* H H^T H */
    if (H != NULL && next != NULL && ws[0] != NULL && ws[1] != NULL
        && ws[2] != NULL) {
        for (iter = 0; iter < MAX_ITER; iter++) {
            update_step(H, W, next, ws, n, k);
            diff = frob_diff_sq(next, H, n, k);
            tmp = H;
            H = next;
            next = tmp;
            if (diff < EPSILON) {
                break;
            }
        }
    } else {
        free_matrix(H);
        H = NULL;
    }

    free_matrix(next);
    free_matrix(ws[0]);
    free_matrix(ws[1]);
    free_matrix(ws[2]);

    return H;
}

/* ---------------------------------------------------------------------- */
/* Standalone program: input, output, main                                */
/* ---------------------------------------------------------------------- */

/*
 * First pass over the input file: counts values and rows (a row ends with
 * '\n'; a missing final newline is tolerated). Sets *n and *d.
 * Returns 0 on success, 1 if the file is empty or not rectangular.
 */
static int count_dims(FILE *fp, int *n, int *d)
{
    long values = 0, rows = 0;
    double val;
    char c;
    int r;

    while ((r = fscanf(fp, "%lf%c", &val, &c)) >= 1) {
        values++;
        if (r == 1 || c == '\n') {
            rows++;
        }
        if (r == 1) {
            break;
        }
    }
    if (rows == 0 || values % rows != 0) {
        return 1;
    }
    *n = (int)rows;
    *d = (int)(values / rows);
    return 0;
}

/*
 * Reads the comma separated points of file_name into a new n x d matrix
 * (two passes: count dimensions, then rewind and fill).
 * Returns NULL on any failure (file cannot be opened, bad format, memory).
 */
static double **read_points(const char *file_name, int *n, int *d)
{
    FILE *fp;
    double **X = NULL;
    char c;
    int i, j, ok = 1;

    fp = fopen(file_name, "r");
    if (fp == NULL) {
        return NULL;
    }
    if (count_dims(fp, n, d) == 0) {
        X = alloc_matrix(*n, *d);
    }
    if (X != NULL) {
        rewind(fp);
        for (i = 0; i < *n && ok; i++) {
            for (j = 0; j < *d && ok; j++) {
                ok = fscanf(fp, "%lf%c", &X[i][j], &c) >= 1;
            }
        }
        if (!ok) {
            free_matrix(X);
            X = NULL;
        }
    }
    fclose(fp);
    return X;
}

/* Prints a rows x cols matrix: '%.4f' values, comma separated, row per line. */
static void print_matrix(double **M, int rows, int cols)
{
    int i, j;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            printf("%.4f%s", M[i][j], (j < cols - 1) ? "," : "\n");
        }
    }
}

/* Computes the matrix of the given goal (sym/ddg/norm); NULL if unknown/failed. */
static double **compute_goal(const char *goal, double **X, int n, int d)
{
    if (strcmp(goal, "sym") == 0) {
        return sym(X, n, d);
    }
    if (strcmp(goal, "ddg") == 0) {
        return ddg(X, n, d);
    }
    if (strcmp(goal, "norm") == 0) {
        return norm(X, n, d);
    }
    return NULL;
}

/*
 * Returns 1 if goal is one of the goals supported by the C program.
 * Lets main fail immediately, before opening the file, when the goal is
 * not one of the supported goals.
 */
static int is_valid_goal(const char *goal)
{
    return strcmp(goal, "sym") == 0 || strcmp(goal, "ddg") == 0
           || strcmp(goal, "norm") == 0;
}

/*
 * Prints the requested matrix and returns 0;
 * on any error prints: "An Error Has Occurred" and returns 1.
 */
int main(int argc, char *argv[])
{
    double **X, **R;
    int n, d;

    if (argc != 3 || !is_valid_goal(argv[1])) {
        printf(ERROR_MSG);
        return 1;
    }
    X = read_points(argv[2], &n, &d);
    if (X == NULL) {
        printf(ERROR_MSG);
        return 1;
    }
    R = compute_goal(argv[1], X, n, d);
    free_matrix(X);
    if (R == NULL) {
        printf(ERROR_MSG);
        return 1;
    }
    print_matrix(R, n, n);
    free_matrix(R);
    return 0;
}
