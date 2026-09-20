"""Independent numpy reference implementation of SymNMF (for tests only).

Usage: python3 tests/reference.py k goal file   (same output as symnmf.py)
"""
import sys

import numpy as np


def sym(X):
    sq = ((X[:, None, :] - X[None, :, :]) ** 2).sum(axis=2)
    A = np.exp(-sq / 2)
    np.fill_diagonal(A, 0.0)
    return A


def ddg(X):
    return np.diag(sym(X).sum(axis=1))


def norm(X):
    A = sym(X)
    d = A.sum(axis=1)
    inv = 1 / np.sqrt(d)
    return A * inv[:, None] * inv[None, :]


def symnmf(X, k, beta=0.5, eps=1e-4, max_iter=300, denom_eps=1e-6):
    W = norm(X)
    H = np.random.uniform(0, 2 * np.sqrt(W.mean() / k), size=(len(W), k))
    for _ in range(max_iter):
        new = H * (1 - beta + beta * (W @ H) / (H @ (H.T @ H) + denom_eps))
        diff = ((new - H) ** 2).sum()
        H = new
        if diff < eps:
            break
    return H


def main():
    np.random.seed(1234)
    k, goal, path = int(float(sys.argv[1])), sys.argv[2], sys.argv[3]
    X = np.loadtxt(path, delimiter=',', ndmin=2)
    funcs = {"sym": sym, "ddg": ddg, "norm": norm}
    M = symnmf(X, k) if goal == "symnmf" else funcs[goal](X)
    for row in M:
        print(",".join("%.4f" % v for v in row))


if __name__ == "__main__":
    main()
