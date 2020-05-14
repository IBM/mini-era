# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

# Settings for RiscV64 platforms.
ifeq ($(TARGET), riscv64)

  TARGET_ARCH := riscv64

  TARGET_TOOLCHAIN_PREFIX := riscv64-unknown-linux-gnu-

  LIBS := -lstdc++ -lpthread -lm -latomic -ldl -lrt
endif
