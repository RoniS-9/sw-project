"""Checks the 40-line function limit (non-blank lines, incl. signature and
comments inside the body) for the submitted C and Python files.

C functions are detected by the project's style: a definition line at
column 0 followed by a line "{" at column 0, ending at a "}" at column 0.
"""
import ast
import os
import sys

LIMIT = 40
ROOT = os.path.join(os.path.dirname(__file__), "..")


def c_functions(path):
    lines = open(path).read().split("\n")
    result, i = [], 0
    while i < len(lines):
        if lines[i] == "{" and i > 0:
            start = i - 1
            while start > 0:
                prev = lines[start - 1].strip()
                if not prev or prev.endswith((";", "*/", "}")) or prev.startswith(("#", "/*", "*")):
                    break
                start -= 1
            end = i
            while lines[end] != "}":
                end += 1
            body = lines[start:end + 1]
            result.append((lines[start].strip(), sum(1 for x in body if x.strip())))
            i = end
        i += 1
    return result


def py_functions(path):
    src = open(path).read()
    lines = src.split("\n")
    result = []
    for node in ast.walk(ast.parse(src)):
        if isinstance(node, ast.FunctionDef):
            body = lines[node.lineno - 1:node.end_lineno]
            result.append((node.name, sum(1 for x in body if x.strip())))
    return result


def main():
    bad = 0
    for name in ("symnmf.c", "symnmfmodule.c", "symnmf.py", "analysis.py"):
        path = os.path.join(ROOT, name)
        funcs = c_functions(path) if name.endswith(".c") else py_functions(path)
        worst = max(funcs, key=lambda f: f[1])
        print("%-15s %2d functions, longest %2d lines (%s)" % (name, len(funcs), worst[1], worst[0]))
        for fname, count in funcs:
            if count > LIMIT:
                print("  OVER LIMIT: %s (%d lines)" % (fname, count))
                bad += 1
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
