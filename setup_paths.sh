#!/bin/bash
####### THESE VARIABLES NEED TO BE SET! #########
#export HPVM_DIR=$HOME/work_dir/hpvm-dssoc/hpvm
#export RISCV_BIN_DIR=$HOME/work_dir/riscv/bin
export RISCV_BIN_DIR=/opt/riscv/bin
export ESP_ROOT=/home/wellman/HPVM/epochs/esp


####### THESE VARIABLES SHOULD NOT NEED ANY MODIFICATION! #########
SH="$(readlink -f /proc/$$/exe)"
if [[ "$SH" == "/bin/zsh" ]]; then
  CUR_DIR="${0:A:h}"
else
  CUR_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
fi

export PATH=$PATH:/RISCV_BIN_DIR
