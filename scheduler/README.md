# Mini-ERA Smart-Scheduler SDK

This is a Software Development Environment for initial development, deployment, and testing of a Smart Scheduler.
This code provides the Mini-ERA (FFT and Viterbi) kernels (i.e. the C-Subset of Mini-ERA) implemented atop a
Smart Scheduler library layer.  Calls to execute the FFT or Viterbi (accelerator functions) are now turned into
calls to the Scheduler to request_execution of a Task (either an FFT_TASK or a VITERBI_TASK) and the
scheduler will then schedule these tasks across the available function-execution hardware (e.g. on a CPU via
a pthread, or on a hardware accelerator where those are implemented).

## Requirements

Mini-ERA has been successfully built and executed using the following set-up:
 - Ubuntu 18.04
 - Ubuntu 16.04

Other platforms should also work; this implementation does NOT include the CV/CNN tensorflow model code, and
thus does not require that support, etc.  This code does require gcc or similar/compatible compiler, an up to date pthreads
library, and the C99 standard.

## Installation and Execution
The installationa nd execution are fairly standard, vit github clone and makefiles.

### Installation

Install by cloning the repository (really a sub-repo of the main Mini-ERA repository):

```
git clone https://github.com/IBM/mini-era.git
cd mini-era/scheduler
```

### Build

The basic Mini-ERA+Scheduler build should be accomplished with a simple make; the makefile also
supports the clean target to facilitate clean-up and full re-compilations.


```
make clean
make
```

The ```make clean``` can be used to ensure that all code is re-built, i.e. in case there are odd time-stamps on the files, etc.
The ```make``` command should produce the ```test-scheduler.exe``` target, which is the core executable that will run the
C-mode Mini-ERA application atop the Scheduler.

### Configuration

The Mini-ERA + Scheduler build supports more than just a basic Linux platform; there is provision to tie this same application
into the EPOCHS ESP-based (https://esp.cs.columbia.edu) Soc platform (currently propotyped on an FPGA).  As such, there
are options to compile this application code within the ESP SoC design environment to cross-compile an output target or
the EPOCHS native RISC-V Linux SoC environment.  There are also some additional configuration capabilites tied in to the overall
make process, and these are controlled in two ways:

1. Build using an explicit Makefile, e.g. to build the local (native to this system) version, invoke ```make -f Makefile.local``` which will use the native gcc and compile to a version that does not include the use of ESP-based SoC Accelerator hardware; to compile explicitly to the ESP-based SoC RISC-V system, build with ```make -f Makefile.riscv```

2. Alter the contents of the ```.config``` file.  The config file contains a set of defines used by the Makefile to produce the proper build style by default.

### The ```.config``` file contents

The config file contains a series of definitions, like #define macros or environment variables, wused to guide the default make behavior.  These contents are:

- DO_CROSS_COMPILATION=y  indicates we are cross-compiling (uses the cross-compiler defined in Makefile)
- COMPILE_TO_ESP=y	  indicates we are compiling to target the ESP RISC-V SoC environment
- CONFIG_ESP_INTERFACE=y  this should always be set -- historical control to choose between some function interfaces.
- CONFIG_FFT_EN=y	  enable FFT Hardware Accelerators
- CONFIG_FFT_FX=32	  indicates FFT accelerators use 32-bit FXP format (can specify 64)
- CONFIG_FFT_BITREV=y	  indicates FFT accelerators include the bit-reverse operation
- CONFIG_VITERBI_EN=y	  enable Viterbi Decode Hardware Accelerators
- CONFIG_KERAS_CV_BYPASS=y	 turns off the Tensorflow code, etc. -- Leave this enabled!
- CONFIG_VERBOSE=y	  turns on a LOT of debugging output
- CONFIG_GDB=y		  indicates compilation should iclude the "-g" flag to retain symbols, etc. which provides for greater debugger support, etc.



### Usage
```
./test-scheduler.exe -h
Usage: ./cmain.exe <OPTIONS>
 OPTIONS:
    -h         : print this helpful usage info
    -o         : print the Visualizer output traace information during the run
    -R <file>  : defines the input Radar dictionary file <file> to use
    -V <file>  : defines the input Viterbi dictionary file <file> to use
    -C <file>  : defines the input CV/CNN dictionary file <file> to use
    -t <trace> : defines the input trace file <trace> to use
    -f <N>     : defines Log2 number of FFT samples
               :      14 = 2^14 = 16k samples (default); 10 = 1k samples
    -v <N>     : defines Viterbi messaging behavior:
               :      0 = One short message per time step
               :      1 = One long  message per time step
               :      2 = One short message per obstacle per time step ** Currently unavailable
               :      3 = One long  message per obstacle per time step ** Currently unavailable
               :      4 = One short msg per obstacle + 1 per time step ** Currently unavailable
               :      5 = One long  msg per obstacle + 1 per time step ** Currently unavailable
```

To actually execute a trace, one must point to the trace repository.  The scheduler sub-directoyr does not include a trace directory itself, but instead uses the one from Mini-ERA.  One can do this in several ways:
1. Copy the ```test-scheduler.exe``` to the (parent) mini-era directory, and run from there
2. Create a soft-link (```ln -s scheduler/test-scheduler.exe .```) in the (parent) mini-era directory, and run from there, therefore always using the most-recently compiled version of ```test-scheduler.exe```
3. Create a soft-link in the scheduler directory to the (parent) Mini-ERA traces directory, and run from the scheduler directory, e.g. ```ln -s ../traces .```   This is done by default and should work IF you cloned the top-level mini-era repository in full.
4. create a local traces directory, and populate it with the required files (see the Mini-ERA README)

## Status

This platform is meant for Scheduler-Library/Layer development and integration, so it is expected to change over
time.  Currently, this is a relatively complete but bare-bones trace version Mini-ERA implementation.
Additional features of the Mini-ERA code, and extensions thereto should also be developed over time.

There are currently some example traces in the ```traces``` subdirectory.  Note that most development of Mini-ERA and
its off-shoots to date has focused around the ```tt02.new``` trace.
 - ```tt00.new``` is a 5000 record illustrative trace
 - ```tt001new``` is a 5000 record illustrative trace
 - ```tt002new``` is a 5000 record trace that includes multiple obstacle vehicles per lane (at times)
 - ```tt003new``` is a 5000 record trace that includes multiple obstacle vehicles per lane (at times)

For additional information, please see the (historic, parent) Mini-ERA master main-line README.



## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)
 - Augusto Veja (ajvega@us.ibm.com)
