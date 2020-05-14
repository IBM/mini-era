# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

TF_DIR = ../../models
TFLIT_DIR = ../../models

TOOL=0

all:
	@echo "INFO: make <tab> for targets"
.PHONY: all

-include models.mk

visualize:
	@echo "INFO: To visualize HDF5 and TFLITE models, you can"
	@echo "INFO:   netron <file.hdf5 | file.tflite>"
	@echo "INFO: or"
	@echo "INFO:   python visualize.py -f <file.tflite> [-i, -o, -a]"
.PHONY: visualize

clean:
	rm -f $(TFLIT_DIR)/*.tflite
.PHONY: clean
