#!/bin/bash
# Packs the submission (NOT part of the submission). Run inside the c-python
# container from the project root:  bash pack.sh <your_id>
# Creates dist/<id>_111111111_project/ with ONLY the submission files and
# dist/<id>_111111111_project.tar.gz. Does not touch the source files.

ID="$1"
if ! [[ "$ID" =~ ^[0-9]{9}$ ]]; then
    echo "usage: bash pack.sh <9-digit id>"
    exit 1
fi
cd "$(dirname "$0")" || exit 1

FILES="symnmf.py symnmf.c symnmfmodule.c symnmf.h analysis.py setup.py Makefile"
NAME="${ID}_111111111_project"

rm -rf "dist/$NAME" "dist/$NAME.tar.gz"
mkdir -p "dist/$NAME"
cp $FILES "dist/$NAME/" || exit 1
cd dist && tar -czvf "$NAME.tar.gz" "$NAME"
