# scripts/env.sh

# repo root = parent of this scripts/ dir
export OTTER_TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

export PATH="$OTTER_TOOLS/programs/riscv_gnu_toolchain/bin:$OTTER_TOOLS/programmer/bin:$OTTER_TOOLS/scripts:$PATH"

umask 022