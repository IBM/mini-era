# Demo-ERA: Demo Version of the Main ERA Workload

This is a top-level driver for the Demo-ERA workload.

The Demo-ERA serves as a stand-alone version of the workload for Colaborative Autonomous Vehicles, intended to drive the
EPOCHS full system design and development methodology through Phase-2 of the DSSoC Program.
The Demo-ERA will serve as teh origination workload from which the EPOCHS Phase-2 SoC will be
developed through application of the full suite of EPOCHS Domain-Specific SoC tools.

Currently, Demo-ERA contains a number of kernels and a simpel pland-and-control type decision module
which is used to update the autonomous vehicle's state in this demo version.
The main kernels modules currently implemented are:
 - The "Radar" kernel, used to model the radar range-finding to the nearest obstacle in a given lane (usually the lane in which the autonomous vehicle is traveling)
 - An H264 decoder, used to decode the input frames from the onboard camera in order to send them through the CV/CNN module
 - A CV/CNN (Computer Vision Object Recognition and Labeling) Module, which identifies the type of obstacle being viewed by the camera
 - An IEEE 802.11p Software-Defined Radio derived Transmit pipeline that supports transmission of signals from the Autonomous Vehicle to other (nearby) vehicles.
 - An IEEE 802.11p Software-Defined Radio derived Receive pipeline that supports reception of signals from other, nearby Autonomous Vehicles
 - An Occupancy Grid Map Creation kernel, used to generate an occupancy grid map representation of the world (obstacles) near the AV
 - An Occupancy Grid-Map Fusion kernel, used to fuse grid maps received from other AVs with the local AV grid map to create a larger and more detaled, shared-intelligence representation of the world.


## System Requirements

Demo-ERA has been successfully built and executed using the following set-up:
 - Ubuntu 18.04
 - Python 3.0
 - Python 2.7
 
Ubuntu 16.04 platforms with older Python versions may also work but have not been tested.
As the Demo-ERA code is encoded using relatively straightforward C (except for the CV/CNN code)
it is expected that most any Linux domain (or other operating system) should be able to
compile at least the "c-only" mode (i.e. the version not including the CV/CNN code) versions.

*Note:* The CV CNN code requires clear delineation of Python 2 or Python 3, and modification of the Makefile to employ one or the other.  The rest of the python code is largely Python 2/3 agnostic.


## Installation and Execution
```
git clone https://github.com/IBM/mini-era.git demo-era
cd demo-era
git checkout phase-2
cd cv/CNN_MIO_KERAS
python mio_dataset.py
```

This last step (`python mio_dataset.py`) has to be run **only once** to create test images as `.npy` files. Then:

### Build

A basic Demo-ERA build is straightforward, and requires only that one execute the default make action in the ```demo-era``` directory.  

```
make clobber
make
```

The ```make clobber``` is added just for surety that all code is re-built, i.e. in case there are odd time-stamps on the files, etc.  The ```make``` command should produce the ```demo-era.exe``` target, which provides the basic means to run a full Demo-ERA simulation.

### Usage
```
./demo-era.exe -h
Usage: ./cmain.exe <OPTIONS>
 OPTIONS:
    -h         : print this helpful usage info
    -o         : print the Visualizer output traace information during the run
    -R <file>  : defines the input Radar dictionary file <file> to use
    -C <file>  : defines the input CV/CNN dictionary file <file> to use
    -H <file>  : defines the input H264 dictionary file <file> to use
    -b         : Bypass (do not use) the H264 functions in this run.
    -t <trace> : defines the input trace file <trace> to use
    -p <N>     : defines the plan-and-control repeat factor (calls per time step -- default is 1)
    -f <N>     : defines which Radar Dictionary Set is used for Critical FFT Tasks
               :      Each Set of Radar Dictionary Entries Can use a different sample size, etc.
    -M <N>     : Use message <N> (0 = 1500 'x' chars, 1 = 1500 '0123456789', 2 = quote)
    -T "S"     : Use message S (No longer than 1500 chars)
    -S <0|1>   : 0=disable 1=enable output of (Xmit and Recv) Messages each time step
    ```

#### Other Targets

Demo-ERA also supports a number of other targets.  One can build these individually, or can build them all and provision a variety of available run types.  The simplest means to build all targets is:
```
make all
```
If one is compiling on a system that does not include the full support for the CV/CNN (Keras, Tensorflow) model, then
one can restrict the build to only produce the Demo-ERA executables that do not utilize those resources.  This is referred to as
the C-subset, or the C-version Demo-ERA (as it does not includethe other languages used in defining the CNN model, etc.).  To build
this set of Demo-ERA executables, the easiest method is to build the "cver" target:
``` make cver
```

The ```make all``` command should compile a number of targets, including:
 - The default, trace-driven Demo-ERA:
   - demo-era.exe : the trace-driven Demo-ERA
   - vdemo-era.exe : trace-driven Demo-ERA in "verbose" mode (includes debug output)
   - gp_demo-era.exe : the trace-driven Demo-ERA set up for ```gprof``` profiling
   - it-demo-era.exe : the demo-era.exe Demo-ERA with support for timing the execution of various elements (and report the timing)
 - The trace-driven Demo-ERA with no use of Keras/Python code:
   - cdemo-era.exe : the trace-driven Demo-ERA with no use of Keras/Python code
   - vcdemo-era.exe : the trace-driven cdemo-era.exe Demo-ERA in "verbose" mode, with no use of Keras/Python code 
   - gp_cdemo-era.exe : the trace-driven Demo-ERA without CNN, and set up for ```gprof``` profiling
   - it-cdemo-era.exe : the cdemo-era.exe Demo-ERA with support for timing the execution of various elements (and report the timing)

### Execution (Example)

Trace-driven Demo-era requires the specification of the input trace (using -t <trace_file>) and also supports the specification of a message modeling behavior for the Viterbi kernel, using -v <N> (where N is an integer).  The corresponding behaviors are:

```
./demo-era.exe -t <trace_name>     (e.g. traces/test_trace1.new)
```

There are a number of additional command-line controls as well; see the ```Usage``` above. 

### Parameter Dependence

The usage defines sets of command-line options, but there are some usage restrictions and comditions that must hold
in order to define a legal (i.e. functionally correct) execution. These are detialed here.

#### FFT Samples and FFT Dictionary

Demo-ERA defaults currently to a 16k-sample FFT, and to the ```traces/norm_radar_16k_dictionary.dfn``` radar dictionary file.
There are some additional radar dictionary files provided in the ```traces``` directory.  One of these contains multiple
complete Radar input sets, one for 1k-sample sand one for 16k-samples in the same dictionary (the ```norm_radar_all_dictionary.dfn```).
One can use this input dictionary, and then use the ```-f``` option to select input set 0 or 1
(where set 0 is the 1k-sample inputs and set 1 is the 16k-sample inputs).  Specification of an input parametere
value for ```-f``` which exceeds the number of sets in the dictionary file results in an error report.

The radar dictionary files and their descriptions:
 - ```norm_radar_16k_dictionary.dfn``` defines normalized inputs, etc. for a 16k-sample FFT 
 - ```norm_radar_01k_dictionary.dfn``` defines normalized inputs, etc. for a  1k-sample FFT
 - ```norm_radar_all_dictionary.dfn``` defines normalized inputs, etc. for a 1k-sample FFT set followed by a 16k-sample FFT 

Thus, legal combinations of the ```-f``` and ```-R``` options are:
 - ```-f 0 -R traces/norm_radar_16k_dictionary``` which is 16k normalized samples
 - ```-f 0 -R traces/norm_radar_01k_dictionary``` which is  1k normalized samples
 - ```-f 0 -R traces/norm_radar_all_dictionary``` which selects the  1k normalized samples set
 - ```-f 1 -R traces/norm_radar_all_dictionary``` which selects the 16k normalized samples set


## Status

The trace version currently uses an input trace to drive the Demo-ERA behaviors, and the computer-vision, Viterbi and radar ranging functionality in the underlying kernels. There are some example traces in the ```traces``` subdirectory.
 - ```test_trace1.new``` is a short trace (117 time steps) that illustrates the funciton of the Demo-ERA, is readable ASCII code, and fairly simple.
 - ```test_trace2.new``` is a slightly longer (1000 time steps) simple illustrative trace
 - ```tt00.new``` is a 5000 record illustrative trace
 - ```tt01.new``` is a 5000 record illustrative trace
 - ```tt02.new``` is a 5000 record trace that includes multiple obstacle vehicles per lane (at times)
 - ```tt03.new``` is a 5000 record trace that includes multiple obstacle vehicles per lane (at times)

There is an example trace (`test_trace.new`) to illustrate the function of the Demo-ERA, and a collection of dictionary files (e.g. `radar_dictionary.dfn`) which are used by the kernels in conjunction with the input trace to drive the proper kernel inputs (in response to the trace inputs).

Notes that all these traces are simple ASCII format, with one time step per line in the file.  One can create a sset trace
by taking the initial N lines of the file, e.g. to create a 500-time-step version of tt02.new one could use:

```
     head -n 500 traces/tt02.new > traces/tt02_0500.new
```

Currently our primary trace for analysis, testing, demos, etc. has been tt02.new (or a 100 or 500 time-step some sub-trace of that trace).


### Trace Format

The trace is a simple ASCII file. The general format of the trace file is a 3-tuple with one set of information per lane (left, center, right) representing information about the contents of that lane for that time-step. The general format of a trace entry is:

```
Xl:yl,Xm:ym,Xr:yr
```

where `X` is a character which identifies a type of obstacle object, and `y` is an unsigned integer representing a distance measure for that object. The `Xl` would be a character identifying an object in the Left lane, at distance `yl` while the `Xm` identifies an object in the `Middle` lane at distance `ym` and the `Xr` indicates an object in the `Right` lane at distance `yr`.  

The "world" in Demo-ERA is viewed as a 2-dimensional space of five-lane highway, where the lanes are arranged left-to-right and the distances are from position zero (which is effectively the back of the car) to some maximum value N which represents the farthest out objects can occur/be tracked.
The lanes are labeled left-to-right as:
 - Left-Hazard Lane
 - Left Lane
 - Middle Lane
 - Right Lane
 - Right-Hazard Lane
and obstacle vehicles are expected to operate only in the Left, Middle and Right lanes (in the trace-driven version)  or across all five lanes (in the simulation version).

<img src="/utils/demo_era.png" width="400">

In this implementation, the objects include:

```
  C - a car
  T - a truck
  P - a pedestrian
  B - a bike
  N - nothing
```

In concert, the distances currently implemented are values between 0 and 550 and represent some currently unspecified unit of distance (though in truth they correspond to the radar kernel's distance measures, which span roughly zero to 500 meters, where 550 represents "infinitely far away" or "no obstale detected"). 
The actual radar inputs (dictionary values) are currently "bucketized" into 50-unit increments, therefore the rada will report distances of 0, 50, 100, 150, ..., 500 or 550 (infinite) for any detected objects.
The following image illustrates how a specific scenario at a given point in time is could be encoded in a trace entry, assuming we are using 50-metere steps of distance (though the trace currently can specify non bucketized distances, e.g. 97, and these are 
converted to the equivalent (currently 50-meter) bucket at run time (e.g. by using ```floor(dist/50)```) when selecting the effective radar input.:

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
 
### The Radar Dictionary Format

This file describes the Radar dictionary format, as defined in `radar_dictionary.dfn`.

The trace dictionary is defined as follows:

```
<S> <E>     - Number of dictionary Sets (```S```) and Entries per Set (```E```)
<L>         - The Log2 of the number of samples (```L```) per entry in the set
<I> <L> <D> - the ID of this entry (```I```), Log2 Samples (```L```) for this entry, ) and Distance (```D```) it represents
<f>         - These are the input data values (float format)
<f>         - There are (```1<<L```) or ```2^L``` per dictionary entry in this set
...
<f>
<I> <L> <D> - the ID of this entry (```I```), Log2 Samples (```L```) for this entry, ) and Distance (```D```) it represents
...
...
<f>
<L>         - The Log2 of the number of samples (```L```) per entry in the next set
<I> <L> <D> - the ID of this entry (```I```), Log2 Samples (```L```) for this entry, ) and Distance (```D```) it represents
...
```

### The CV Dictionary Format

This version of the Demo-ERA does not use or require a CV CNN Dictionary file.  Note that this may change as we move forward, but currently the dictionary functionality is incorporated into the Python CV CNN kernel code.


### The H264 Dictionary Format

This version of the Demo-ERA does not use or require an H264 Dictionary file (yet).  There is provision in place to utilize one in the future.

### The IEEE 802.11p Xmit/Recv Pipelines

The current implementation of Demo-ERA does not use dictionaries for the IEEE 802.11p pipelines.
Presently, the message to be transmitted can be selected from an internal list (see the ```-M``` option)
or specified on the command line as a string (see the ```-T``` option in ```Usage``` above).
The receive pipeline curretly receives the same message that is being transmitted.


## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)
 - Augusto Veja (ajvega@us.ibm.com)
