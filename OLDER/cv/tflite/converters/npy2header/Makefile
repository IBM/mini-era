# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

help:
	@echo "INFO: make <TAB> to show the options"
.PHONY:

convert:
	python npy2header.py \
		--test_images ../../data/model/test_images.npy \
		--test_labels ../../data/model/test_labels.npy \
		--output_dir ../../data/model \
		--model_name model \
		--range_begin 0 \
		--range_end 16
.PHONY: convert
