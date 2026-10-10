"""Checks the 40-line function limit for the submitted C and Python files.

Counted: non-blank lines from the documentation comment directly above a
function (forum, 6 Oct 2026: it "counts as a function line") through its
last line, including the signature and comments inside the body.

C functions are detected by the project's style: a definition line at
column 0 followed by a line "{" at column 0, ending at a "}" at column 0.
"""
import ast
import os
import sys

LIMIT = 40
ROOT = os.path.join(os.path.dirname(__file__), "..")


def c_doc_start(lines, start):
    """Index of the first line of the /* ... */ block directly above line
    start, or start itself if the line above is not the end of a comment."""
    if start > 0 and lines[start - 1].strip().endswith("*/"):
        start -= 1
        while start > 0 and not lines[start].strip().startswith("/*"):
            start -= 1
    return start


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
            name = lines[start].strip()
            start = c_doc_start(lines, start)
            end = i
            while lines[end] != "}":
                end += 1
            body = lines[start:end + 1]
            result.append((name, sum(1 for x in body if x.strip())))
            i = end
        i += 1
    return result


def py_functions(path):
    src = open(path).read()
    lines = src.split("\n")
    result = []
    for node in ast.walk(ast.parse(src)):
        if isinstance(node, ast.FunctionDef):
            first = node.lineno - 1
            while first > 0 and lines[first - 1].strip().startswith("#"):
                first -= 1                      # "#" comment lines above the def
            body = lines[first:node.end_lineno]
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
            elif count >= LIMIT - 2:
                print("  near limit: %s (%d lines)" % (fname, count))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
