# Mini-ERA: Simplified Version of the Main ERA Workload

This is a top-level driver for the Mini-ERA workload.

## Requirements

Mini-ERA has been successfully built and executed using the following set-up:
 - Ubuntu 18.04
 - Python 3.0
 
Ubuntu 16.04 platforms with older Python versions (e.g. 2.7) may also work but have not been tested.


## Installation and Execution

```
git clone https://github.com/IBM/mini-era.git
cd mini-era
cd cv/CNN_MIO_KERAS
python3 mio_dataset.py
```

This last step (`python3 mio_dataset.py`) has to be run **only once** to create test images as `.npy` files. Then:

```
make allclean
make
./main.exe <trace_name>     (e.g. traces/test_trace1.new)
```

 
## Status

This version currently uses an input trace to drive the Mini-ERA behaviors, and the computer-vision, Viterbi and radar ranging functionality in the underlying kernels.

There is an example trace (`test_trace.new`) to illustrate the funciton of the Mini-ERA, and a collection of dictionary files (e.g. `radar_dictionary.dfn`) which are used by the kernels in conjunction with the input trace to drive the proper kernel inputs (in response to the trace inputs).


## Trace Format

The trace is a simple ASCII file. The general format of the trace file is a 3-tuple with one set of information per lane (left, center, right) representing information about the contents of that lane for that time-step. The general format of a trace entry is:

```
X:y,X:y,X:y
```

where `X` is a character which identifies a type of obstacle object, and `y` is an unsigned integer representing a distance measure for that object. The "world" is viewed as a 2-dimensional space where the lanes are arranged left-to-right and the distances are from position zero (which is effectively the back of the car) to some maximum value N which represents the farthest out objects can occur/be tracked.

<img src="/utils/mini_era.png" width="400">

In this implementation, the objects include:

```
  C - a car
  T - a truck
  P - a pedestrian
  B - a bike
  N - nothing
```

In concert, the distances currently implemented are values between 0 and 550, corresponding to 50-meter increments from 0 to 500 (with the final value, 550, being "Infinity"). The following image illustrates how a specific scenario at a given point in time is encoded in a trace entry:

```
  Distance | Left | Cntr | Right|
  -------------------------------
  |  500  |      |      |      |
  |  450  |      |      |      |
  |  400  |      |  T   |      |
  |  350  |      |      |      |
  |  300  |  C   |      |      |
  |  250  |      |      |      |
  |  200  |      |      |      |
  |  150  |      |      |      |
  |  100  |      |      |  P   |
  |   50  |      |      |      |
  |    0  |      |      |      |

Corresponding trace entry:  C:300,T:400,P:100 
```

## Dictionary Files

Each kernel also uses a dictionary which translates conditions (e.g. the distance of an object) into the appropriate inputs for the underlying kernel. Each dictionary file has a statically-defined name (for now) and a similar encoding:

```
   <n> = number of dictionary entries (message types)
```

For each dictionary entry:

```
   <kernel-specific input parameters>
```

The following sections lay out the specific dictionary file formats per kernel.


### The Viterbi Dictionary Format

This file describes the Viterbi dictionary format, as defined in `vit_dictionary.dfn`.

The trace dictionary is defined as follows:

```
   <n> = number of dictionary entries (message types)
```

For each dictionary entry:

```
   n1 n2 n3 n4 n5 : OFDM parms: 
   m1 m2 m3 m4 m5 : FRAME parms:
   x1 x2 x3 ...   : The message bits (input to decode routine)
```

```
4   	              - There are 4 messages in the dictionary
1 48 24 0 13  	    - The OFDM paramteers for the first message (which is "Msg0")
32 12 10 576 288    - The FRAME parameters for the first message (which is "Msg0")
0 0 0 0 0 0 1 1 ... - The input bits for the first message (last bit ENDs this dictionary entry)
1 48 24 0 13  	    - The OFDM paramteers for the second message (which is "Msg1")
32 12 10 576 288    - The FRAME parameters for the second message (which is "Msg1")
0 0 0 0 0 0 1 1 ... - The input bits for the second message (last bit ENDs this dictionary entry)
...
0 0 0 0 0 0 1 1 ... - The input bits for the fourth message (last bit ENDs this and all dictionary entries)
```
 
### The Radar Dictionary Format

This file describes the Radar dictionary format, as defined in `radar_dictionary.dfn`.

The trace dictionary is defined as follows:

```
<n>         - Number of dictionary entries
<id> <dist> - the ID of this entry (sequential number?) and Distance it represents
<f>         - These are the input data values (float format)
<f>         - There are RADAR_N of them (1<<14) per dictionary entry
...
<f>
<id> <dist> - the ID of the next entry, and Distance it represents
...
```

### The CV Dictionary Format

This file describes the CV dictionary format, as defined in `cv_dictionary.dfn`.

The trace dictionary is defined as follows:

```
<n>         - Number of Dictionary Entries
<id> <obj>  - the ID of this entry (sequential number?) and label_t it represents
<u>         - These are the input image data values (unsigned format)
<u>         - There are ?N? of them (currently just 32 as a placeholder) per dictionary entry
...         - These are probably image pixel values; may need to be RGB values?
<u>
<id> <dist> - the ID of the next entry, and Distance it represents
...
```

## Requirements

The Mini-ERA applications currently consists of four major components:

 - The main Mini-ERA driver (scaffolding)
 - The CV kernel
 - The Viterbi kernel
 - The Radar kernel

Each kernel is implemented in a separate subdirectory space, consistent with their name/function (e.g. Viterbi is in `viterbi`, etc.). Most of the Mini-ERA is implemented in standard C code using GCC compilation. The CV CNN code, however, is a Keras implementation of a CNN and requires
the system suport Keras python input, etc.

<img src="/utils/block_diagram.png" width="600">


## Installation and Building

To install the Mini-ERA workload, clone the git repository:

```
  git clone https://github.com/IBM/mini-era
  cd mini-era/cv/CNN_MIO_KERAS
  export PYTHONPATH=`pwd`  (path_to_mini_era/cv/CNN_MIO_KERAS)
  python3 mio_dataset.py
```

To build Mini-ERA::

```
  cd mini-era
  make all
```
This will build all the necessary programs; the mini-era main program(s), the
utilities programs, etc. Some of these can be built independently, e.g. the
trace generator includes compile-time parameters (at present) and thus might
need to be recompiled to change the output trace.

To build the trace-generation program:

```
  cd utils
  make tracegen
```

To generate an input trace:
```
  ./tracegen > <target_trace_file>
```

Note that some example traces have been provided, in the traces directory.
The '''test_trace1.new''' is a short trace, and the
'''tt00.new''' and '''tt01.new''' and '''tt02.new''' files are longer example traces.

## Invocation and Usage

The main mini-era program is invoked using the program main.exe, but there is also a debugging (verbose) version named vmain.exe.
The invocation also requires an input trace file to be specifed:
```
  main.exe traces/test_trace1.new
```

The visualizer can also be used to visualize the operation of the simulation.  The visualizer sits in the visualizer subdirectory, and currently requires its own version of the trace to operate. Please see the visualizer README.md file in the visualizer subdirectory.

To drive the visualizer, one needs to produce a Visualizer trace.  The mini-era program can produce these traces.  Currently, the method to generate a Visualizer input trace from a Mini-ERA run (itself driven by a Mini-ERA input trace) is to run the verbose version of Mini-ERA and pull out the Visualizer trace data from that output stream.  This is easily done as follows:
```
  ./vmain.exe traces/test_trace1.new | grep VizTrace | sed 's/VizTrace: //' > visualizer/traces/viz_tet_trace1.new
```

## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)
 - Nandhini Chandramoorthy (Nandhini.Chandramoorthy@ibm.com)
 - Augusto Veja (ajvega@us.ibm.com)
