#!/usr/bin/env bash
# programs/scripts/env.sh

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

export OTTER_TOOLS="$REPO_ROOT"
export PATH="$REPO_ROOT/programs/riscv_gnu_toolchain/bin:$REPO_ROOT/programmer/bin:$REPO_ROOT/scripts:$PATH"

umask 022