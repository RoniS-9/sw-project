#!/bin/bash
# Test suite (NOT part of the submission). Run inside the c-python container
# from the project root:  bash tests/run_tests.sh
# Builds everything, compares C / Python / numpy-reference outputs, checks
# error handling, runs valgrind on the C program and the 40-line checker.

cd "$(dirname "$0")/.." || exit 1
OUT=$(mktemp -d)
PASS=0
FAIL=0

ok()  { PASS=$((PASS + 1)); echo "  ok   $1"; }
bad() { FAIL=$((FAIL + 1)); echo "  FAIL $1"; }

echo "== build"
make clean >/dev/null
if make 2>&1 | grep -q 'warning:'; then bad "make warnings"; else ok "make (no warnings)"; fi
[ -x symnmf ] && ok "symnmf built" || bad "symnmf not built"
# Remove the build cache so the extension is really recompiled: otherwise
# setuptools prints nothing and the check below passes without testing anything.
rm -rf build ./*.so
python3 setup.py build_ext --inplace > "$OUT/build" 2>&1
if [ $? -ne 0 ]; then
    bad "setup.py build failed"; tail -5 "$OUT/build"
elif ! grep -q -- '-c symnmfmodule.c' "$OUT/build"; then
    bad "setup.py did not recompile (check would be vacuous)"
elif grep -qE '(warning|error):' "$OUT/build"; then
    # gcc writes real diagnostics as "warning:" / "error:"; the compiler flags
    # themselves (e.g. -Werror=format-security) must not match.
    bad "setup.py warnings/errors"; grep -E '(warning|error):' "$OUT/build"
else
    ok "setup.py build_ext (no warnings)"
fi

echo "== sym/ddg/norm: C program == Python module == numpy reference"
for f in tests/data/*.txt; do
    for g in sym ddg norm; do
        name="$g $(basename "$f")"
        ./symnmf "$g" "$f" > "$OUT/c" || { bad "$name (C exit code)"; continue; }
        python3 symnmf.py 2 "$g" "$f" > "$OUT/py" || { bad "$name (py exit code)"; continue; }
        python3 tests/reference.py 2 "$g" "$f" > "$OUT/ref"
        cmp -s "$OUT/c" "$OUT/py" || { bad "$name (C != Python)"; continue; }
        python3 tests/compare.py "$OUT/c" "$OUT/ref" 1e-4 && ok "$name" || bad "$name (vs reference)"
    done
done

echo "== symnmf goal vs numpy reference (same seed and algorithm)"
for spec in "3 tests/data/blobs_30x2.txt" "2 tests/data/tiny_4x3.txt" \
            "3 tests/data/hw1_input_1.txt" "7 tests/data/hw1_input_2.txt"; do
    set -- $spec
    python3 symnmf.py "$1" symnmf "$2" > "$OUT/py" || { bad "symnmf k=$1 $2 (exit code)"; continue; }
    python3 tests/reference.py "$1" symnmf "$2" > "$OUT/ref"
    python3 tests/compare.py "$OUT/py" "$OUT/ref" 1e-3 && ok "symnmf k=$1 $(basename "$2")" \
        || bad "symnmf k=$1 $2"
done
python3 symnmf.py 3.0 symnmf tests/data/blobs_30x2.txt > "$OUT/a"
python3 symnmf.py 3 symnmf tests/data/blobs_30x2.txt > "$OUT/b"
cmp -s "$OUT/a" "$OUT/b" && ok "k=3.0 same as k=3" || bad "k=3.0 differs from k=3"

echo "== analysis.py"
for spec in "3 tests/data/blobs_30x2.txt" "3 tests/data/hw1_input_1.txt" "7 tests/data/hw1_input_2.txt"; do
    set -- $spec
    res=$(python3 analysis.py "$1" "$2")
    if echo "$res" | grep -qE '^nmf: -?[0-9]+\.[0-9]{4}$' && echo "$res" | grep -qE '^kmeans: ' \
        && echo "$res" | grep -qE '^ari: ' && [ "$(echo "$res" | wc -l)" -eq 3 ]; then
        ok "analysis k=$1 $(basename "$2"): $(echo $res)"
    else
        bad "analysis k=$1 $2: $res"
    fi
done

echo "== error handling"
E="An Error Has Occurred"
K="Incorrect number of clusters!"
D=tests/data/blobs_30x2.txt
expect() {  # expect "<message>" <command...>
    local msg="$1"; shift
    local res; res=$("$@" 2>&1); local code=$?
    if [ "$res" = "$msg" ] && [ $code -eq 1 ]; then ok "$* -> $msg"; else bad "$* -> got '$res' (exit $code)"; fi
}
expect "$E" ./symnmf
expect "$E" ./symnmf sym
expect "$E" ./symnmf sym $D extra
expect "$E" ./symnmf symnmf $D
expect "$E" ./symnmf foo $D
expect "$E" ./symnmf sym tests/data/no_such_file.txt
expect "$E" python3 symnmf.py 3 sym
expect "$E" python3 symnmf.py 3 foo $D
expect "$E" python3 symnmf.py 3 sym tests/data/no_such_file.txt
expect "$K" python3 symnmf.py 1 symnmf $D
expect "$K" python3 symnmf.py 0 sym $D
expect "$K" python3 symnmf.py 30 symnmf $D
expect "$K" python3 symnmf.py 3.5 symnmf $D
expect "$K" python3 symnmf.py -3 ddg $D
expect "$K" python3 symnmf.py abc norm $D
expect "$K" python3 symnmf.py 4 sym tests/data/tiny_4x3.txt
expect "$E" python3 analysis.py 3
expect "$E" python3 analysis.py 3 tests/data/no_such_file.txt
expect "$K" python3 analysis.py 1 $D
expect "$K" python3 analysis.py 30 $D

echo "== valgrind (C program)"
if command -v valgrind >/dev/null; then
    for g in sym ddg norm; do
        valgrind --leak-check=full --error-exitcode=9 --errors-for-leak-kinds=all -q \
            ./symnmf $g tests/data/hw1_input_2.txt > /dev/null 2> "$OUT/vg"
        [ $? -eq 0 ] && ! [ -s "$OUT/vg" ] && ok "valgrind $g clean" || { bad "valgrind $g"; cat "$OUT/vg"; }
    done
    valgrind --leak-check=full --error-exitcode=9 -q ./symnmf foo $D > /dev/null 2> "$OUT/vg"
    [ $? -eq 1 ] && ! [ -s "$OUT/vg" ] && ok "valgrind error path clean" || bad "valgrind error path"
else
    echo "  (valgrind not found, skipped)"
fi

echo "== 40-line function limit"
python3 tests/check_lines.py && ok "all functions <= 40 lines" || bad "function length"

rm -rf "$OUT"
echo
echo "passed: $PASS  failed: $FAIL"
[ $FAIL -eq 0 ]
