# Mini-ERA: Simplified Version of the Main ERA Workload

This is a top-level driver for the Mini-ERA workload.

## Requirements

Mini-ERA has been successfully built and executed using the following set-up:
 - Ubuntu 18.04
 - Python 3.0
 - Python 2.7
 
Ubuntu 16.04 platforms with older Python versions may also work but have not been tested.

*Note:* The CV CNN code requires clear delineation of Python 2 or Python 3, and modification of the Makefile to employ one or the other.  The rest of the python code is largely Python 2/3 agnostic.


## Installation and Execution
```
git clone https://github.com/IBM/mini-era.git
cd mini-era
cd cv/CNN_MIO_KERAS
python mio_dataset.py
```

This last step (`python mio_dataset.py`) has to be run **only once** to create test images as `.npy` files. Then:

### Build

A basic Mini-ERA build is straightforward, and requires only that one execute the default make action in the ```mini-era``` directory.  

```
make allclean
make
```

The ```make allclean``` is added just for surety that all code is re-built, i.e. in case there are odd time-stamps on the files, etc.  The ```make``` command should produce the ```main.exe``` target, which provides the basic means to run a Mini-ERA simulation.

### Usage
```
./main.exe -h
Usage: ./cmain.exe <OPTIONS>
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

#### Other Targets

Mini-ERA also supports a number of other targets.  One can build these individually, or can build them all and provision a variety of available run types.  The simplest means to build all targets is:
```
make all
```

The primary update is that Mini-ERA now supports two modes of execution: 
 - a trace-driven mode (the "classic" mode) where the simulation is driven by an input trace of obstacle positions per simulation time step 
 - a new, developing mode which uses on-board environment simulation to both introduce new obstacle vehicles and track the positions, etc. during simulation
This new simulation version is developed from the trace generation utility, and provides a greater range of behaviors to the autonomous vehicle ("red car") during a Mini-ERA run.

The ```make all``` command should compile a number of targets:
 - The default, trace-driven Mini-ERA:
   - main.exe : the trace-driven Mini-ERA
   - vmain.exe : trace-driven Mini-ERA in "verbose" mode (includes debug output)
 - The trace-driven Mini-ERA with no use of Keras/Python code:
   - cmain.exe : the trace-driven Mini-ERA with no use of Keras/Python code
   - vcmain.exe : the trace-driven Mini-ERA in "verbose" mode, with no use of Keras/Python code 
 - The default, trace-driven Mini-ERA:
   - sim_main.exe : the base Mini-ERA code running in a simulation (not trace-driven) mode
   - vsim_main.exe : the simulation-mode Mini-ERA in "verbose" mode (includes debug output)
 - The default, trace-driven Mini-ERA:
   - csim_main.exe : the simulation-mode Mini-ERA with no use of Keras/Python code
   - vcsim_main.exe : the simulation-mode Mini-ERA in "verbose" mode, with no use of Keras/Python code

### Execution (Example)
The usage for the trace-driven Mini-ERA and simulation-mode Mini-ERA differ slightly.  Here we will give two examples, one for each mode.

#### Trace Mode:

Trace-driven Mini-era requires the specification of the input trace (using -t <trace_file>) and also supports the specification of a message modeling behavior for the Viterbi kernel, using -v <N> (where N is an integer).  The corresponding behaviors are:

```
./main.exe -t <trace_name>     (e.g. traces/test_trace1.new)
```

By using the -v <N> behavior controls, one can simulate the Viterbi messaging work that could load the system when either operating with a single global (environmental) messsage model, with a pure swarm collaboration model (where each other nearby vehicle sends a message) and in a hybrid that includes both kinds of messaging.  The message length also allows one to consider the effect of larger and small message payload decoding on the overall Viterbi run-time impact.

Recall that the usage is:
```
./main.exe -h
Usage: ./cmain.exe <OPTIONS>
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


#### Simulation Mode:

Trace-driven Mini-era requires the specification of the input trace (using -t <trace_file>) and also supports the specification of a message modeling behavior for the Viterbi kernel, using -v <N> (where N is an integer).  The corresponding behaviors are:

```
./sim_main.exe 
```


The simulation-mode usage is:
```
./csim_main.exe -h
Usage: ./csim_main.exe <OPTIONS>
 OPTIONS:
    -h         : print this helpfule usage info
    -o         : print the Visualizer output traace information during the run
    -s <N>     : Sets the max number of time steps to simulate
    -r <N>     : Sets the rand random number seed to N
    -v <N>     : defines Viterbi messaging behavior:
               :      0 = One short message per time step
               :      1 = One long  message per time step
               :      2 = One short message per obstacle per time step
               :      3 = One long  message per obstacle per time step
               :      4 = One short msg per obstacle + 1 per time step
               :      5 = One long  msg per obstacle + 1 per time step
```

Note that in simulation mode, there is no option to specify a trace (the '-t' of trace-mode) as there is no need for or use of a trace.  There are two additional options in simulation-mode: the '-s' which indicates the number of time steps to simulate (which is analogous to the trace length) and '-r' which sets the seed value (and unsigned int) for the C ```rand()``` function used in the simulation.  Both modes support the '-v' and '-o' options.

## Status

The trace version currently uses an input trace to drive the Mini-ERA behaviors, and the computer-vision, Viterbi and radar ranging functionality in the underlying kernels. There are some example traces in the ```traces``` subdirectory.
 - ```test_trace1.new``` is a short trace (117 time steps) that illustrates the funciton of the Mini-ERA, is readable ASCII code, and fairly simple.
 - ```test_trace2.new``` is a slightly longer (1000 time steps) simple illustrative trace
 - ```tt00.new``` is a 5000 record illustrative trace
 - ```tt001new``` is a 5000 record illustrative trace
 - ```tt002new``` is a 5000 record trace that includes multiple obstacle vehicles per lane (at times)
 - ```tt003new``` is a 5000 record trace that includes multiple obstacle vehicles per lane (at times)

There are also a collection of dictionary files (e.g. ```radar_dictionary.dfn```) in the ```traces``` subdirectory, which are used by the kernels to drive the proper kernel inputs given the desired inputs from the Mini-ERA environment.


### Trace Format

The trace is a simple ASCII file. The general format of the trace file is a 3-tuple with one set of information per lane (left, center, right) representing information about the contents of that lane for that time-step. The general format of a trace entry is:

```
Xl:yl,Xm:ym,Xr:yr
```

where `X` is a character which identifies a type of obstacle object, and `y` is an unsigned integer representing a distance measure for that object. The `Xl` would be a character identifying an object in the Left lane, at distance `yl` while the `Xm` identifies an object in the `Middle` lane at distance `ym` and the `Xr` indicates an object in the `Right` lane at distance `yr`.  

The "world" in Mini-ERA is viewed as a 2-dimensional space of five-lane highway, where the lanes are arranged left-to-right and the distances are from position zero (which is effectively the back of the car) to some maximum value N which represents the farthest out objects can occur/be tracked.
The lanes are labeled left-to-right as:
 - Left-Hazard Lane
 - Left Lane
 - Middle Lane
 - Right Lane
 - Right-Hazard Lane
and obstacle vehicles are expected to operate only in the Left, Middle and Right lanes (in the trace-driven version)  or across all five lanes (in the simulation version).

<img src="/utils/mini_era.png" width="400">

In this implementation, the objects include:

```
  C - a car
  T - a truck
  P - a pedestrian
  B - a bike
  N - nothing
```

In concert, the distances currently implemented are values between 0 and 550 and represent some currently unspecified unit of distance (thoguh in truth they correspond to the radar kernel's distance measures, which span roughly zero to 500 meters, and 550 represents "infinitely far away" or "no obstale detected"). 
The following image illustrates how a specific scenario at a given point in time is encoded in a trace entry, assuming we are using 50-metere steps of distance (though in fact the trace currently can contain arbitrary unsigned integer values for distance; these are converted to 50-meter "buckets" (i.e. ```floor(dist/50)``` when determining the radar input).:

```
  Distance| Left | Cntr | Right|
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

This version of the Mini-ERA does not use or require a CV CNN Dictionary file.  Note that this may change as we move forward, but currently the dictionary functionality is incorporated into the Python CV CNN kernel code.


## More Details, Additional Utilities, etc. 

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
  python mio_dataset.py
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
  main.exe -t traces/test_trace1.new
```

The visualizer can also be used to visualize the operation of the simulation.  The visualizer sits in the visualizer subdirectory, and currently requires its own version of the trace to operate. Please see the visualizer README.md file in the visualizer subdirectory.

To drive the visualizer, one needs to produce a Visualizer trace.  The mini-era program can produce these traces.  Currently, the method to generate a Visualizer input trace from a Mini-ERA run (itself driven by a Mini-ERA input trace) is to run the verbose version of Mini-ERA and pull out the Visualizer trace data from that output stream.  This is easily done as follows:
```
  ./vmain.exe -t traces/test_trace1.new | grep VizTrace | sed 's/VizTrace: //' > visualizer/traces/viz_tet_trace1.new
```

## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)
 - Augusto Veja (ajvega@us.ibm.com)
