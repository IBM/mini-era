#!/bin/bash

# Save the current configuration file
mv .config .config.save

echo "Building All-Software..."
sed 's/##/#/g' build_riscv.config | sed 's/##/#/' | sed 's/##/#/' | sed 's/#DO_CROSS_COMPILATION=y/DO_CROSS_COMPILATION=y/' | sed 's/CONFIG_FFT_EN=y/#CONFIG_FFT_EN=y/' | sed 's/CONFIG_VITERBI_EN=y/#CONFIG_VITERBI_EN=y/' > .config
make clean; make 


echo "Building Hardware Viterbi..."
sed 's/##/#/g' build_riscv.config | sed 's/##/#/' | sed 's/##/#/' | sed 's/#DO_CROSS_COMPILATION=y/DO_CROSS_COMPILATION=y/' | sed 's/CONFIG_FFT_EN=y/#CONFIG_FFT_EN=y/' | sed 's/#CONFIG_VITERBI_EN=y/CONFIG_VITERBI_EN=y/' > .config
make clean; make

echo "Building Hardware FFT (1) ..."
sed 's/##/#/g' build_riscv.config | sed 's/##/#/' | sed 's/##/#/' | sed 's/#DO_CROSS_COMPILATION=y/DO_CROSS_COMPILATION=y/' | sed 's/#CONFIG_FFT_EN=y/CONFIG_FFT_EN=y/' | sed 's/CONFIG_VITERBI_EN=y/#CONFIG_VITERBI_EN=y/' | sed 's/#CONFIG_WHICH_FFT_TYPE=1/CONFIG_WHICH_FFT_TYPE=1/' | sed 's/CONFIG_WHICH_FFT_TYPE=2/#CONFIG_WHICH_FFT_TYPE=2/' > .config
make clean; make

echo "Building All-Hardware FFT (1)..."
sed 's/##/#/g' build_riscv.config | sed 's/##/#/' | sed 's/##/#/' | sed 's/#DO_CROSS_COMPILATION=y/DO_CROSS_COMPILATION=y/' | sed 's/#CONFIG_FFT_EN=y/CONFIG_FFT_EN=y/' | sed 's/#CONFIG_VITERBI_EN=y/CONFIG_VITERBI_EN=y/' | sed 's/#CONFIG_WHICH_FFT_TYPE=1/CONFIG_WHICH_FFT_TYPE=1/' | sed 's/CONFIG_WHICH_FFT_TYPE=2/#CONFIG_WHICH_FFT_TYPE=2/' > .config
make clean; make

echo "Building Hardware FFT (1) ..."
sed 's/##/#/g' build_riscv.config | sed 's/##/#/' | sed 's/##/#/' | sed 's/#DO_CROSS_COMPILATION=y/DO_CROSS_COMPILATION=y/' | sed 's/#CONFIG_FFT_EN=y/CONFIG_FFT_EN=y/' | sed 's/CONFIG_VITERBI_EN=y/#CONFIG_VITERBI_EN=y/' | sed 's/CONFIG_WHICH_FFT_TYPE=1/#CONFIG_WHICH_FFT_TYPE=1/' | sed 's/#CONFIG_WHICH_FFT_TYPE=2/CONFIG_WHICH_FFT_TYPE=2/' > .config
make clean; make

echo "Building All-Hardware FFT (1)..."
sed 's/##/#/g' build_riscv.config | sed 's/##/#/' | sed 's/##/#/' | sed 's/#DO_CROSS_COMPILATION=y/DO_CROSS_COMPILATION=y/' | sed 's/#CONFIG_FFT_EN=y/CONFIG_FFT_EN=y/' | sed 's/#CONFIG_VITERBI_EN=y/CONFIG_VITERBI_EN=y/' | sed 's/CONFIG_WHICH_FFT_TYPE=1/#CONFIG_WHICH_FFT_TYPE=1/' | sed 's/#CONFIG_WHICH_FFT_TYPE=2/CONFIG_WHICH_FFT_TYPE=2/' > .config
make clean; make

# Restore the original cofiguration file
mv .config.save .config
