"""SymNMF command line interface.

Usage: python3 symnmf.py k goal file_name.txt
  goal: symnmf - final H of the SymNMF optimization (n x k)
        sym / ddg / norm - similarity / degree / normalized similarity (n x n)
All computations are done by the C extension module symnmfmodule.
"""
import sys

import numpy as np

import symnmfmodule

GOALS = ("symnmf", "sym", "ddg", "norm")
ERROR_MSG = "An Error Has Occurred"
K_ERROR_MSG = "Incorrect number of clusters!"


def fail(msg=ERROR_MSG):
    """Print msg and terminate the program with exit code 1."""
    print(msg)
    sys.exit(1)


def parse_natural(s):
    """Parse s as a non-negative integer; integer-valued decimals such as
    "3.0" are accepted. Return the integer, or None if s is not such a number.
    """
    if s is None or s == "":
        return None
    int_part, _, frac_part = s.partition('.')
    if int_part == "" or not int_part.isdigit():
        return None
    if frac_part != "" and (not frac_part.isdigit() or frac_part.strip('0')):
        return None
    return int(int_part)


def parse_k(s):
    """Parse the k argument; exit with the clusters error if it is not a
    natural number greater than 1 (the k < N check needs the data)."""
    k = parse_natural(s)
    if k is None or k <= 1:
        fail(K_ERROR_MSG)
    return k


def load_points(file_name):
    """Read the comma separated points of file_name as a list of float lists.
    Exit with the general error if the file cannot be read."""
    try:
        data = np.loadtxt(file_name, delimiter=',', ndmin=2, dtype=float) # return result with at least two dimensions
    except Exception:
        fail()
    if data.size == 0:
        fail()
    return data.tolist()


def check_k(k, n):
    """Exit with the clusters error unless 1 < k < n."""
    if not 1 < k < n:
        fail(K_ERROR_MSG)


def init_h(W, k):
    """Random initial H (n x k): entries drawn uniformly from
    [0, 2 * sqrt(m / k)], where m is the mean of all entries of W."""
    m = np.mean(np.array(W))
    upper = 2 * np.sqrt(m / k)
    return np.random.uniform(0, upper, size=(len(W), k)) # datapoints (rows) x clusters (columns)


def run_symnmf(X, k):
    """Full SymNMF: W = norm(X) (in C), random H init (in Python), then the
    C optimization. Returns the final H as a list of float lists."""
    W = symnmfmodule.norm(X)
    H0 = init_h(W, k)
    return symnmfmodule.symnmf(H0.tolist(), W)


def compute(goal, X, k):
    """Return the matrix requested by goal for the points X."""
    if goal == "symnmf":
        return run_symnmf(X, k)
    if goal == "sym":
        return symnmfmodule.sym(X)
    if goal == "ddg":
        return symnmfmodule.ddg(X)
    return symnmfmodule.norm(X) # goal == "norm"


def print_matrix(M):
    """Print M with 4 decimal places, comma separated, one row per line."""
    for row in M:
        print(",".join("%.4f" % float(v) for v in row))


def parse_args(argv):
    """Validate the command line and return (k, goal, X)."""
    if len(argv) != 4:
        fail()
    k = parse_k(argv[1])
    goal = argv[2]
    if goal not in GOALS:
        fail()
    X = load_points(argv[3])
    check_k(k, len(X))
    return k, goal, X


def main():
    """Entry point: parse arguments, compute the goal and print it."""
    np.random.seed(1234)
    k, goal, X = parse_args(sys.argv)
    try:
        result = compute(goal, X, k)
    except Exception:
        fail()
    print_matrix(result)


if __name__ == "__main__":
    main()
