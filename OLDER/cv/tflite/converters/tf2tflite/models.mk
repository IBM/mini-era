# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

model:
ifeq ($(TOOL), 1)
	tflite_convert --keras_model_file=$(TF_DIR)/$@/$@.h5 --output_file=$(TFLIT_DIR)/$@/$@.tflite
else
	python ./converter.py $(TF_DIR)/$@/$@.h5 $(TFLIT_DIR)/$@/$@.tflite
endif
	netron $(TF_DIR)/$@/$@.h5 $(TFLIT_DIR)/$@/$@.tflite
.PHONY: model
