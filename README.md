# Mini-ERA: Simplified Version of the Main ERA Workload

This is a top-level driver for the Mini-ERA Workload

## Status

This version currently uses input traces to drive the Viterbi and Radar (FFT)
functionality in the underlying kernels, and provides a scaffolding for the CV
kernel (i/e/ reads the objects dictionary and trace and consumes them; also reports
the object labels from the trace inputs).

There are example trace for these kernels (test_trace.vit, test_trace.rad, and test_trace.cv)
which are equal length, along with legally-sized dictionaries for CV (object_dictionary.dfn) and
the radar (radar_dictionary.dfn).  The dictionary for the viterbi is defined in the start of the trace.
The radar dictionary is quite large, so we put it in a separate file; we assume the same may be true
for the CV code.

# Trace Formats

The traces are simple ASCII files.

## The Viterbi Trace Format
This file describes the Viterbi trace format, using the example
vit_trace_dict_v04.txt as an illustrative example.

The trace format is:
    The trace dictionary
    The stream of trace time-step entries.

The trace dictionary is defined as follows:
   <n> = number of dictionary entries (message types)
 For each dictionary entry:
   n1 n2 n3 n4 n5 : OFDM parms: 
   m1 m2 m3 m4 m5 : FRAME parms:
   x1 x2 x3 ...   : The message bits (input to decode routine)

Then after all dictionary entries, there is a stream of per-time-step, per-lane status indicators:
   t1_0 t2_0 t3_0 : Message per-lane (t1 = left, t2 = middle, t3 = right) for time step 0
   t1_1 t2_1 t3_1 : Message per-lane (t1 = left, t2 = middle, t3 = right) for time step 1

Each time-step of the trace, the viterbi_iterate routines reads in the trace values for the left, middle and right lanes
(i.e. which message if the autonomous car is in the left, middle or right lane).
WE MUST KNOW WHICH LANE'S TRACE TO USE THIS TIME STEP otherwise we can report inconsistent state
[ Should the car's lane be an input or a global variable? We currently assume a global: GLOBAL_CURRENT_LANE = 0, 1, 2 (left, middle right) ]

The exmaple vit_trace_desc_v04.txt reads as follows (annotated here with some comments)
4   	    - There are 4 messages in the dictionary:
1 48 24 0 13  	    - The OFDM paramteers for the first message (which is "Msg0")
32 12 10 576 288    - The FRAME parameters for the first message (which is "Msg0")
0 0 0 0 0 0 1 1 ... - The input bits for the first message (last bit ENDs this dictionary entry)
1 48 24 0 13  	    - The OFDM paramteers for the second message (which is "Msg1")
32 12 10 576 288    - The FRAME parameters for the second message (which is "Msg1")
0 0 0 0 0 0 1 1 ... - The input bits for the second message (last bit ENDs this dictionary entry)
...
0 0 0 0 0 0 1 1 ... - The input bits for the fourth message (last bit ENDs this and all dictionary entries)
Then the trace:
LANES:
L M R	- LEFT-LANE-MSG		MIDDLE-LANE-MSG		RIGHT-LANE-MSG		SUMMARY			Picture
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 1 2 	- "OK-to-go-right"	"Ok-to-go-right"	"Ok-to-go-left"		Left Lane Full		|X| | |
1 1 2 	- "OK-to-go-right"	"Ok-to-go-right"	"Ok-to-go-left"		Left Lane Full		|X| | |
1 1 2 	- "OK-to-go-right"	"Ok-to-go-right"	"Ok-to-go-left"		Left Lane Full		|X| | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 2 2 	- "OK-to-go-right"	"Ok-to-go-left"		"Ok-to-go-left"		Right Lane Full		| | |X|
3 2 3 	- "OK-to-go-NONE"	"Ok-to-go-left"		"Ok-to-go-NONE"		Right/Mid Lanes Full	| |X|X|
3 3 3 	- "OK-to-go-NONE	"Ok-to-go-NONE"		"Ok-to-go-NONE"		ALL Lanes Full  	|X|X|X|
3 1 3 	- "OK-to-go-NONE"	"Ok-to-go-right"	"Ok-to-go-NONE"		Left/Mid Lanes Full	|X|X| |
1 1 2 	- "OK-to-go-right"	"Ok-to-go-right"	"Ok-to-go-NONE"	    	Left Lane Full 		|X| | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |
1 2 2 	- "OK-to-go-right"	"Ok-to-go-left"		"Ok-to-go-left"		Right Lane Full		| | |X|
1 1 2 	- "OK-to-go-right"	"Ok-to-go-right"	"Ok-to-go-left"		Left Lane Full		|X| | |
1 0 2 	- "OK-to-go-right"	"Ok-left-or-right"	"Ok-to-go-left"		All Lanes Open		| | | |

 
## The Radar Inputs
The radar kernel uses a separate Dictionary and a trace file

The Dictionary format:
<n> - Number of Dictionary Entries
<id> <dist> - the ID of this entry (sequential number?) and Distance it represents
<f> These are the input data values (float format)
<f>   There are RADAR_N of them (1<<14) per dictionary entry
...
<f>
<id> <dist> - the ID of the next entry, and Distance it represents
...

The trace format is a 3-tuple of dictionaryt entries per lane per time-step:

<ln> <cn> <rn> - the left-lane Dictionary ID, the center-lane ID and the right-lane ID
<ln> <cn> <rn> - the IDs for the next time step
...

## The CV Kernel Inputs
The cv kernel uses a separate Dictionary and a trace file

The Dictionary format:
<n> - Number of Dictionary Entries
<id> <obj> - the ID of this entry (sequential number?) and label_t it represents
<u> These are the input image data values (unsigned format)
<u>   There are ?N? of them (currently just 32 as a placeholder) per dictionary entry
...   These are probably image pixel values; may need to be RGB values?
<u>
<id> <dist> - the ID of the next entry, and Distance it represents
...

The trace format is a 3-tuple of dictionaryt entries per lane per time-step:

<ln> <cn> <rn> - the left-lane Dictionary ID, the center-lane ID and the right-lane ID
<ln> <cn> <rn> - the IDs for the next time step
...


## Contacts, etc.

Maintainers:
 - Augusto Veja : ajvega@us.ibm.com
 - J-D Wellman : wellman@us.ibm.com

