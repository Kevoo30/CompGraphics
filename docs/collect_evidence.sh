#!/usr/bin/env bash
# Usage: from the repo root, after building:  bash docs/collect_evidence.sh [path/to/people_are_watching]
set -e

EXE="${1:-build/people_are_watching}"
mkdir -p docs/evidence
"$EXE" --software --shot && mv -f screenshot_software.bmp docs/evidence/software.bmp
"$EXE" --shot          && mv -f screenshot_opengl.bmp   docs/evidence/opengl.bmp
echo "Saved to docs/evidence/"
