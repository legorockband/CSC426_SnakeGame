# programs/scripts/env.sh

export OTTER_TOOLS="$HOME/otter_tools"

export PATH="$OTTER_TOOLS/riscv_gnu_toolchain/bin:$OTTER_TOOLS/programmer/bin:$OTTER_TOOLS/scripts:$PATH"

hash -r
umask 022