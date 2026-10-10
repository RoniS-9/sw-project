# SymNMF Clustering

Final project for Software Project. Implements Symmetric Non-negative Matrix
Factorization (SymNMF) for clustering in C, exposes it to Python through a C
extension module, and compares its results to K-means.

## Files
| File | Purpose |
|---|---|
| `symnmf.c`, `symnmf.h` | Algorithm core: similarity, degree and normalized matrices, H optimization, and the standalone C program |
| `symnmfmodule.c` | Python/C API wrapper exposing `sym`, `ddg`, `norm`, `symnmf` |
| `symnmf.py` | Python command line interface |
| `analysis.py` | Compares SymNMF and K-means (silhouette score, ARI) |
| `setup.py`, `Makefile` | Build the extension and the C program |
| `tests/` | Test suite |

## Build
```
make
python3 setup.py build_ext --inplace
```

## Usage
```
./symnmf <sym|ddg|norm> <input.txt>
python3 symnmf.py <k> <symnmf|sym|ddg|norm> <input.txt>
python3 analysis.py <k> <input.txt>
```
Input: comma-separated points, one per line. Output: matrices with 4 decimal places.

## Tests
```
bash tests/run_tests.sh
```
Compares the C program, the Python module and an independent numpy reference,
and checks error handling, memory (valgrind) and function length.
