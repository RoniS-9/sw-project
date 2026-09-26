#ifndef SYMNMF_H
#define SYMNMF_H

/*
 * shared interface of the SymNMF C implementation.
 *
 * A matrix is represented as double** : an array of row pointers that all
 * point into one contiguous data block (see alloc_matrix / free_matrix).
 */

#define MAX_ITER 300      /* maximum number of H update iterations */
#define EPSILON 1e-4      /* convergence threshold on ||H(t+1)-H(t)||_F^2 */
#define BETA 0.5          /* update step parameter */
#define DENOM_EPS 1e-12   /* avoids /0 (value suggested on the forum) */

/* Allocates a zero-initialized rows x cols matrix. Returns NULL on failure. */
double **alloc_matrix(int rows, int cols);

/* Frees a matrix created by alloc_matrix. Accepts NULL. */
void free_matrix(double **m);

/* Similarity matrix A (n x n) of the n points of X (n x d). */
double **sym(double **X, int n, int d);

/* Diagonal degree matrix D (n x n) of the n points of X (n x d). */
double **ddg(double **X, int n, int d);

/* Normalized similarity matrix W = D^-1/2 A D^-1/2 (n x n). */
double **norm(double **X, int n, int d);

/*
 * Runs the SymNMF optimization starting from H0 (n x k) with the normalized
 * similarity matrix W (n x n). Returns a newly allocated final H (n x k),
 * or NULL on allocation failure. H0 and W are not modified.
 */
double **symnmf(double **H0, double **W, int n, int k);

#endif
