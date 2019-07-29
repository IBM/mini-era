# Trailing spaces in these variables will cause problems
#
ROOT			= /home/david/git/needle
ARCH			= x86_64
LLVM_VERSION	= 3.8
BITCODE_REPO	= $(ROOT)/examples/$(ARCH)
LLVM_OBJ		= /home/david/git/needle/llvm-3.8/bin
NEEDLE_OBJ		= /home/david/git/needle/build/bin
NEEDLE_LIB		= /home/david/git/needle/build/lib
HELPER_LIB		= $(NEEDLE_LIB)/helpers.bc
DATA			= $(ROOT)/examples/inputs

