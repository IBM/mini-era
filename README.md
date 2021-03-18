# Mini-ERA-HPVM: HPVM port of Mini-ERA

This is the HPVM port of the Mini-ERA workload.

This distribution is intended to operate within the Docker container image with which it is distributed.
This README assumes that you have not altered the locations of files, etc. in that image.  Should you alter the 
directory structure, etc. you will most likely need to alter the contents of the setup_paths.sh (see below).

## Contents

Mini-ERA-HPVM includes:

- HPVM (internal repository branch snapshot)
- ApproxHPVM (internal repository branch snapshot)
- GCC cross compiler tools for RISC-V, currently taken from the Columbia University ESP environment
- Pre-compiled versions of the ESP libraries (used to support compilation to and use of ESP-derived hardware accelerators)

## Installation and Execution
The Docker build process should have resulted in a fully-populated image.  The contents are in 3 major subdirectories:
- approxhpvm-nvdla
- hpvm-epochs
- mini-era


You should no need to do anything within the approxhpvm-nvdla or hpvm-epochs directories; those are the tools directories.
The min-era directory holds the actual Mini-ERA workload (set up to build on HPVM).


### Setup required paths
In order to successfully build Mini-ERA, certain paths need to set using the `setup_paths.sh` script.
You should be able to simply source the setup_paths.sh script directly, unless you have altered the default directory structure.
If you have altered the default directory structure, modify `setup_paths.sh` to include the correct paths for the following variabls:
* `HPVM_DIR` should point to the HPVM repo: `$(PATH_TO_HPVM_REPO)/hpvm`
* `APPROXHPVM_DIR` should point to the ApproxHPVM repo: `$(PATH_TO_APPROXHPVM_REPO)`
* `RISCV_BIN_DIR` should point to the bin folder for the cross-compiler: `$(PATH_TO_RISCV_TOOLCHAIN_BINARIES)`

### Build

To build the HPVM version of Mini-ERA: 

1. After modifying the setup script as described above, source it using `source ./setup_paths.sh`
    - *Note: The scripts must be sourced using `source` because it sets up environment variables that will be needed by the Makefiles.*
2. Build for desired target:
    * For native architecture: `make` will generate the binary `miniera-hpvm-seq`
    * For epochs0 (risc-v host with fft, viterbi and NVLDA accelerators): `make epochs` will generate the binary `miniera-hpvm-epochs`
    * For all-software risc-v version: `make riscv` will generate the binary `miniera-hpvm-riscv`
*Note that when building `riscv` and `epochs` target, an `ld` error will appearing saying that the eh_table_hdr will not be created.
This error can be ignored, the binary is still being generated.
3. To clean the build: `make clean`

*Note: the build must be cleaned before invoking make with a different target!  This make clean will also remove previously compiled targets, unless you move or rename them.

### Usage
```
./miniera-hpvm-* -h
Usage: ./miniera-hpvm-* <OPTIONS>
 OPTIONS:
    -h         : print this helpfule usage info
    -o         : print the Visualizer output traace information during the run
    -t <trace> : defines the input trace file to use
    -v <N>     : defines Viterbi messaging behavior:
               :      0 = One short message per time step
               :      1 = One long  message per time step
               :      2 = One short message per obstacle per time step
               :      3 = One long  message per obstacle per time step
               :      4 = One short msg per obstacle + 1 per time step
               :      5 = One long  msg per obstacle + 1 per time step
```

### Execution (Example - locally)

Trace-driven Mini-era requires the specification of the input trace (using -t <trace_file>) and also supports the specification of a message modeling behavior for the Viterbi kernel, using -v <N> (where N is an integer).  The corresponding behaviors are:

```
./miniera-hpvm-seq -t traces/tt02.new
```

By using the -v <N> behavior controls, one can simulate the Viterbi messaging work that could load the system when either operating with a single global (environmental) messsage model, with a pure swarm collaboration model (where each other nearby vehicle sends a message) and in a hybrid that includes both kinds of messaging.  The message length also allows one to consider the effect of larger and small message payload decoding on the overall Viterbi run-time impact.

## Running on the EPOCHS SoC FPGA

The miniera-hpvm-epochs executable can be run on an EPOCHS SoC (or FPGA emulation of an EPOCHS SoC) which includes at least one FFT, one Viterbi, 
and one NVDLA CV/CNN accelerator.  This executable has been tested and confirmed to work. 
Specific details for running the executable on your own hardware or FPGA may depend on your environment; look for additional documentation for that.

## More Information
For more information about the Mini-ERA application, please visit the main IBM-Resaerch repository for the project: https://github.com/IBM/mini-era.
