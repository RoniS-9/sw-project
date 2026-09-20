"""Usage: python3 tests/compare.py file_a file_b tol
Exit 0 if both files hold matrices of the same shape whose entries differ by
at most tol; otherwise print the largest difference and exit 1."""
import sys

import numpy as np

a = np.loadtxt(sys.argv[1], delimiter=',', ndmin=2)
b = np.loadtxt(sys.argv[2], delimiter=',', ndmin=2)
tol = float(sys.argv[3])
if a.shape != b.shape:
    print("shape mismatch %s vs %s" % (a.shape, b.shape))
    sys.exit(1)
diff = np.abs(a - b).max()
if diff > tol:
    print("max diff %.6g > %g" % (diff, tol))
    sys.exit(1)
