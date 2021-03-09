# Mini-ERA-HPVM: HPVM port of Mini-ERA

This is the HPVM port of the Mini-ERA workload.

## Requirements

Mini-ERA-HPVM Requirements:

- HPVM (internal repository branch `hpvm-epochs0` located [here](https://gitlab.engr.illinois.edu/llvm/hpvm/-/tree/hpvm-epochs0))
    * Refer to [HPVM README](https://gitlab.engr.illinois.edu/llvm/hpvm/-/blob/hpvm-epochs0/README.md) for set up instructions.
    *Note: During installation, make sure target is set to X86;RISCV to be able to target the EPOCHS-0 RISC-V processor.*
- ApproxHPVM (internal repository branch `approx_hpvm_nvdla` located [here](https://gitlab.engr.illinois.edu/llvm/hpvm/-/tree/approx_hpvm_nvdla).
    * Refer to [ApproxHPVM README](https://gitlab.engr.illinois.edu/llvm/hpvm/-/blob/approx_hpvm_nvdla/README.md) for set up instructions.
- GCC cross compiler for RISC-V, can be installed using ESP as follows:
    * Clone ESP repository using: `git clone --recursive https://github.com/sld-columbia/esp.git`
    * Checkout the epochs branch: `cd esp && git checkout epochs`
    * Invoke the cross-compiler installation script: `./utils/scripts/build_riscv_toolchain.sh`

*Note: A pre-built version of the ESP libraries is provided in this repository.*

## Installation and Execution

### Checkout Mini-ERA repository
```
git clone https://github.com/IBM/mini-era.git
cd mini-era
git checkout hpvm
```

### Setup required paths
In order to successfully build Mini-ERA, certain paths needs to set in the `setup_paths.sh` script.
To do that, modify `setup_paths.sh` to include the correct paths for the following variabls:
* `HPVM_DIR` should point to the HPVM repo: `$(PATH_TO_HPVM_REPO)/hpvm`
* `APPROXHPVM_DIR` should point to the ApproxHPVM repo: `$(PATH_TO_APPROXHPVM_REPO)`
* `RISCV_BIN_DIR` should point to the bin folder for the cross-compiler: `$(PATH_TO_RISCV_TOOLCHAIN_BINARIES)`

### Build

To build the HPVM version of Mini-ERA: 

1.After modifying the setup script as described above, source it using `source ./set_paths.sh`
    - *Note: The scripts must be sourced using `source` because it sets up environment variables that will be needed by the Makefiles.*
6. Build for desired target:
    * For native architecture: `make` will generate the binary `miniera-hpvm-seq`
    * For epochs0 (risc-v host with fft, viterbi and NVLDA accelerators): `make epochs` will generate the binary `miniera-hpvm-epochs`
    * For all-software risc-v version: `make riscv` will generate the binary `miniera-hpvm-riscv`
*Note that when building `riscv` and `epochs` target, an `ld` error will appearing saying that the eh_table_hdr will not be created.
This error can be ignored, the binary is still being generated.
7. To clean the build: `make clean`

*Note: the build must be cleaned before invoking make with a different target!

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

### Execution (Example)

Trace-driven Mini-era requires the specification of the input trace (using -t <trace_file>) and also supports the specification of a message modeling behavior for the Viterbi kernel, using -v <N> (where N is an integer).  The corresponding behaviors are:

```
./main.exe -t <trace_name>     (e.g. traces/test_trace1.new)
```

By using the -v <N> behavior controls, one can simulate the Viterbi messaging work that could load the system when either operating with a single global (environmental) messsage model, with a pure swarm collaboration model (where each other nearby vehicle sends a message) and in a hybrid that includes both kinds of messaging.  The message length also allows one to consider the effect of larger and small message payload decoding on the overall Viterbi run-time impact.

## More Information
For more information about the Mini-ERA application, please visit the main IBM-Resaerch repository for the project: https://github.com/IBM/mini-era.
