# Mini-ERA: Simplified Version of the Main ERA Workload

This is a top-level driver for the Mini-ERA Workload

## Status

This version currently uses an input trace to drive the Mini-ERA behaviors, and 
the computer-vision, viterbi and radar ranging 
functionality in the underlying kernels.

There is an example trace (test_trace.new) to illustrate the funciton of the mini-era,
and a collection of dictionary files (e.g. radar_dictionary.dfn) which are used by 
the kernels in conjunction with the input trace to drive the proper kernel inputs
(in response to the trace inputs).

# Trace Format

The trace is a simple ASCII file.
The general format of the trace file is a 3-tuple with one set of information per lane (left, center, right)
representing information about the contents of that lane for that time-step.
The general format of a trace entry is:
```
Xy Xy Xy
```
where X is a character which identifies a type of obstacle object, and y is an unsigned integer representing a 
distance measure for that object.  The "world" is viewed as a 2-dimensional space where the lanes are arrayed
left-to-right and the distances are from position zero (which is effectively the back of the car)
to some maximum value N which represents the fathest out objects can occur/be tracked.

In this implementation, the objects include:
```
  C - a car
  T - a truck
  P - a pedestrian
  B - a bus
  N - nothing
```
In concert, the distances currently implemented are values between 0 and 11, 
corresponding to "50-meter" increments from 0 to 500 (with the final value being "Infinity").

To illustrate, the following image and associated trace are shown:
```
  Distance | Left | Cntr | Right|
  -------------------------------
  |  10    |      |      |      |
  |   9    |      |      |      |
  |   8    |      |  T   |      |
  |   7    |      |      |      |
  |   6    |  C   |      |      |
  |   5    |      |      |      |
  |   4    |      |      |      |
  |   3    |      |      |      |
  |   2    |      |      |  P   |
  |   1    |      |      |      |
  |   0    |      |      |      |

TRACE ENTRY:  C6 T8 P2 
```

# Dictionary Files

Each kernel also uses a Dictionary which translates conditions (e.g. the distance of an object) 
into the appropriate inputs for the underlying kernel.  Each Dictionary file has a statically-defined 
name (for now) and a similar encoding:
```
   <n> = number of dictionary entries (message types)
```
 For each dictionary entry:
```
   <kernel-specific input parameters>
```
The following sections lay out the specific Dictionary File formats per kernel.

## The Viterbi Dictionary Format
This file describes the Viterbi dictionary format, as defined in ```vit_dictionary.dfn```.

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
4   	    - There are 4 messages in the dictionary:
1 48 24 0 13  	    - The OFDM paramteers for the first message (which is "Msg0")
32 12 10 576 288    - The FRAME parameters for the first message (which is "Msg0")
0 0 0 0 0 0 1 1 ... - The input bits for the first message (last bit ENDs this dictionary entry)
1 48 24 0 13  	    - The OFDM paramteers for the second message (which is "Msg1")
32 12 10 576 288    - The FRAME parameters for the second message (which is "Msg1")
0 0 0 0 0 0 1 1 ... - The input bits for the second message (last bit ENDs this dictionary entry)
...
0 0 0 0 0 0 1 1 ... - The input bits for the fourth message (last bit ENDs this and all dictionary entries)
```
 
## The Radar Dictionary Format
The radar kernel uses a Dictionary specified in ```radar_dictionary.dfn```

The Dictionary format:
```
<n> - Number of Dictionary Entries
<id> <dist> - the ID of this entry (sequential number?) and Distance it represents
<f> These are the input data values (float format)
<f>   There are RADAR_N of them (1<<14) per dictionary entry
...
<f>
<id> <dist> - the ID of the next entry, and Distance it represents
...
```
## The CV Kernel Inputs
The cv kernel uses a Dictionary specified in ```cv_dictionary.dfn```

The Dictionary format:
```
<n> - Number of Dictionary Entries
<id> <obj> - the ID of this entry (sequential number?) and label_t it represents
<u> These are the input image data values (unsigned format)
<u>   There are ?N? of them (currently just 32 as a placeholder) per dictionary entry
...   These are probably image pixel values; may need to be RGB values?
<u>
<id> <dist> - the ID of the next entry, and Distance it represents
...
```

# Requirements
The mini-era applications currently consists of 4 main parts:
 - The main Mini-ERA driver (scaffolding)
 - The CV kernel
 - The Viterbi kernel
 - The Radar kernel

Each kernel is implemented in a separate subdirectory space, consistent
with their name/function, e.g. Viterbi is in viterbi, etc.

Most of the Mini-ERA is implemented in standard C code using GCC compilation.
The CV CNN code, however, is a Keras implementation of a CNN and requires
the system suport Keras python input, etc.

# Installation/Building

To install the mini-era workload, clone the git repository
```
  git clone https://github.com/IBM/mini-era
```
Then to build, use the built-in Makefile:
```
  cd mini-era
  make
```
To build the trace-generation program:
```
  make tracegen
```

And to make a trace:
```
  ./tracegen | grep TRACE_LINE | awk '{print $2,$3,$4;}' > <target_trace_file>
```
Note that this should reproduce the ```test_trace.new``` file.

# Programs

The current distribution primarily consists of the top-level mini-era scaffolding application, 
and includes a developing trace-generation program.

# Contacts, etc.

Maintainers:
 - Augusto Veja : ajvega@us.ibm.com
 - J-D Wellman : wellman@us.ibm.com

