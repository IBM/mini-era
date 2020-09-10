# ERAv3 C Application

This code is intended to replicate ERAv3 functionality
by translating portions of the current ERAv3 full application, which uses
ROS (or another autonomous vehicle simulator) and the GnuRadio IEEE802.11p pipeline,s
into a simple C program which our tools can better analyze.
This should include both the transmit and receive functional pipelines, and the "swarm intelligence"
  functionality (i.e. map mfusion, etc.) and additional elements over time.

This code serves as a first "Stake in the ground" implementation, which is conformant to the
previous mini-era (https://github.com/IBM/mini-era) formulation.
Currently, this code includes the IEEE 802.11p WiFi transceiver
functionality translated from the GnuRadio environment to a more standard,
stand-alone C implementation.

## Requirements

This code should operate properly on any system which can currently run the ERAv3 workload.
It should actually run on almost any recent Linux system -- it only requires ISO C99 std build
(e.g. using a recent GCC compiler/run-time).

## Installation and Execution
```
git clone https://github.com/IBM/mini-era.git
cd mini-era/erav3-app
make
```
  
## Usage

The standard build creates three primary executables:
 - ```xmit_erav3c``` which is the IEEE802.11p transmit function
 - ```recv_erav3c``` which is the IEEE802.11p treceiver function
 - ```erav3c``` which is the full IEEE802.11p transmit -> receive loop
It also builds a set of alternate-interface, supplemental executables that use an
alternative interface (coded to compatibility with the ESP Viterbi decoder hardware):
 - ```xmit_erav3c_esp``` which is the IEEE802.11p transmit function using ESP interfaces
 - ```recv_erav3c_esp``` which is the IEEE802.11p treceiver function using ESP interfaces
 - ```erav3c_esp``` which is the full IEEE802.11p transmit -> receive loop using ESP interfaces
These executables should be functionally identical, i.e. the same inputs yield the same exact
outputs, etc.  These ESP versions are intended to act as a functional verification of the
workload when set up to utilize the ESP hardware accelerators.

Finally, the build also generates a full set of "verbose" versions of these executables, which
include a lot of debugging (printf type) outputs:
 - ```verbose_xmit_erav3c``` which is ```xmit_erav3c``` with lots of debug type output
 - ```verbose_recv_erav3c``` which is ```recv_erav3c``` with lots of debug type output
 - ```verbose_erav3c``` which is the ```erav3c``` with lots of debug type output
 - ```verbose_xmit_erav3c_esp``` which is ```xmit_erav3c``` using ESP interfaces, and with lots of debug type output
 - ```verbose_recv_erav3c_esp``` which is ```recv_erav3c``` using ESP interfaces, and with lots of debug type output
 - ```verbose_erav3c_esp``` which is the ```erav3c``` using ESP interfaces, and with lots of debug type output

Each of these three main programs has similar usage; help information is accesible
for any of them via the ```-h``` option.

### erav3c

 The ```erav3c``` encodes a complete transmit-receive loop, by employing
 the ```xmit_erav3c`` code which then feeds its output (encoded message)
 to the ```recv_erav3c``` code, which decodes the
 received message back to ascii text.
 
```
./erav3c -h
Usage: ./erav3c <OPTIONS>
 OPTIONS:
    -h         : print this helpful usage info
    -s <N>     : Run the simulation for <N> time steps
    -m <N>     : Use message <N> (0 = 1500 'x' chars, 1 = 1500 '0123456789', 2 = quote)
    -T "S"     : Use message S (No longer than 1500 chars)
    -M <0|1>   : 0=disable 1=enable output of Messages (input and output) per time step
    -x <0|1>   : 0=disable 1=enable output of XMIT output per time step
    -r <0|1>   : 0=disable 1=enable output of RECV output per time step
```
The ```-s``` option indicates a number of "time steps" to run, which really indicates
the number of times to repeat the translation process.  This is useful for profiling runs
to generate a longer run-time and thus more data samples, etc.

The ```-m``` option selects from among the built-in messages.  See the section on messages below for details.

The ```-T``` option allows a user to type in (within quotes) an ascii message up to 1500 characters long, which the code will then encode.

The ```-M``` option turns off the printing of the original message each time step,
the ```-x``` option turns off the printing of the xmit message each time step, and
the ```-r``` option turns off the printing of the received message each time step.
These message outputs give visual confirmation of proper operation, but turning them off
removes this test-type output and streamlines operation just a bit further.


### xmit_erav3c

The ```xmit_erav3c`` encodes the outgoing message encoder functionality.   Given a message,
this code encodes it in a manner consistent with the IEEE 802.11p protocol.

```
./xmit_erav3c -h
Usage: ./xmit_erav3c <OPTIONS>
 OPTIONS:
    -h         : print this helpful usage info
    -s <N>     : Run the simulation for <N> time steps
    -m <N>     : Use message <N> (0 = 1500 'x' chars, 1 = 1500 '0123456789', 2 = quote)
    -T "S"     : Use message S (No longer than 1500 chars)
    -M <0|1>   : 0=disable 1=enable output of Messages (input and output) per time step
    -x <0|1>   : 0=disable 1=enable output of XMIT output per time step
    -o <FN>    : Output the encoded message data to file <FN>
```
The ```-s``` option indicates a number of "time steps" to run, which really indicates
the number of times to repeat the translation process.  This is useful for profiling runs
to generate a longer run-time and thus more data samples, etc.

The ```-m``` option selects from among the built-in messages.  See the section on messages below for details.

The ```-T``` option allows a user to type in (within quotes) an ascii message up to 1500 characters long, which the code will then encode.

The ```-x``` option turns off the printing of the xmit message each time step.

The ```-o``` option allows the user to provide a target file name, and the output
of the encoded message will be written to that file (for later use, e.g. as input to
a ```recv_erav3c``` program run).

### recv_erav3c
The ```recv_erav3c`` decodes an incoming message using the decoder functionality.
Given an incoming encoded  message, this code decodes it in a manner consistent
with the IEEE 802.11p protocol back to an ASCII text message.

```
./recv_erav3c -h
Usage: ./recv_erav3c <OPTIONS>
 OPTIONS:
    -h         : print this helpful usage info
    -s <N>     : Run the simulation for <N> time steps
    -f <FN>    : Use file <FN> as input encoded message source
    -M <0|1>   : 0=disable 1=enable output of Messages (input and output) per time step
    -r <0|1>   : 0=disable 1=enable output of RECV output per time step
```
The ```-s``` option indicates a number of "time steps" to run, which really indicates
the number of times to repeat the translation process.  This is useful for profiling runs
to generate a longer run-time and thus more data samples, etc.

The ```-f``` option specifies a file name that holds the input encoded message data.

The ```-M``` option turns off the printing of the original message each time step,
the ```-r``` option turns off the printing of the received message each time step.
These message outputs give visual confirmation of proper operation, but turning them off
removes this test-type output and streamlines operation just a bit further.

## Design Details

This code is derived from the GnuRadio IEEE 802.11p protocol code,
derived from GnuRadio's IEEE802.11 pipelines, implemented by
Bastian Bloessl ad et. al. (https://github.com/bastibl/gr-ieee802-11).

The GnuRadio code has been translated from the GnuRadio environment and
C++ into simpler C code, that can run stand-alone, and then has been integrated into
a test framework derived from the Mini-ERA ```kernels_api``` design.
We should really fill this in more; this is a sketch of the operations of the transmit


### TRANSMIT:
  1. Form a message : input string up to 1500 characters : char[1501] (terminating '\0')?
  2. WiFi-MAC : gr-ieee802-11/mac.cc
  3. Wifi-Mapper : gr-ieee802-11/mapper.cc
  4. Packet-Header Generator : gnuradio/gr-digital/lib/packet_headergenerator_bb_impl.cc
  5a. Chunks-to-Symbols : gnuradio/gr-digital/lib/chunks_to_symbols_impl.*
  5b. Chunks-to-Symbols : gr-ieee802-11/chunks_to_symbols_impl.cc
  6. Tagged-Stream-Mux : gnuradio/gr-blocks/lib/tagged_stream_mux_impl.cc  :: <concatenate: header from 5a and data symbols from 5b> :
  7. OFDM-Carrier-Allocator : gnuradio/gr-digital/lib/ofdm_carrier_allocator_cvc_impl.cc
  8. iFFT : gnuradio/gr-fft (uses FFTW lib) : 64-size Inverse FFT
  9. OFDM-Cycle-Prefixer : gnuradio/gr-digital/lib/ofdm_cyclic_prefixer_impl.cc 
  X. OUTPUT : <some number of complex numbers?>

### RECEIVE:
  1. Delay is just a samples delay function (shift register)
  2. Complex Conjugate (volk) : a+ib -> a-ib  (i.e. reverse sign of imaginary part)
  3. Complex to Mag^2 : a+ib -> (a^2 + b^2)  
  4. Multiply : Complex Multiply
  5. Moving Average :  gr-ieee802-11/moving_average_XX_impl.cc.t 
  6. Complex to Mag   : a+ib -> SQRT(a^2 + b^2)  
  7. Divide : FP Divide
  8. WIFI-Sync-Short : gr-ieee802-11/sync_short.cc
  9. WIFI-Sync-Long : gr-ieee802-11/sync_long.cc
 10. Stream-to-Vector : gnuradio/gr-blocks/lib/streams_to_vector_impl.cc 
 11. FFT : gnuradio/gr-fft (uses FFTW lib) : 64-size FFT
 12. WIFI-Frame-Equalizer : gr-ieee802-11/frame_equalizer_impl.cc
 13. WIFI-Decode-MAC : gr-ieee802-11/decode_mac.cc
  X. OUTPUT : the message payload? (1500 symbols)


## Status

This release is a "first-functional" release of the IEEE 802-11 pipelines,
adapted from the GnuRadio versions, implemented here in C code.  The program allows
one to specify a number of time steps of execution, but each time step will currently
transmit and receive the same message (which may be useful for profiling, but otherwise is
not much more interesting than a single time step).

This code has been lightly tested, primarily for a single time-step using the four messages that
currently are encoded in the ```main.c``` code as an ```xmit_messages``` dictionary, and
from which the ```-m``` option selects a message.

More development is needed and expected, including integration of additional new
kernels representing other functionality of the latest ERA V3 design.

## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)


