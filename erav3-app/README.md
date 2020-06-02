# ERAv3 C Application

This code is intended to replicate ERAv3 functionality
by translating portions of the current ERAv3 full application, which uses
ROS (or another autonomous vehicle simulator) and the GnuRadio IEEE802.11p pipeline,s
into a simple C program which our tools can better analyze.
This should include both the transmit and receive functional pipelines, and the "swarm intelligence"
  functionality (i.e. map mfusion, etc.) and additional elements over time.

This code serves as a first "Stake in the ground" implementation, which is conformant to the
previous mini-era (https://github.com/IBM/mini-era) formulation.

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

The standard build creates two executables: ```erav3c``` and ```verbose_erav3c```
which have the exact same functionality, but the ```verbose_erav3c``` also outputs (a lot of)
debugging information/messages (to standard out).


```
./erav3c -h
Usage: ./erav3c.exe <OPTIONS>
 OPTIONS:
    -h         : Print this helpful usage info
    -s <N>     : Sets the max number of time steps to simulate
    -m <N>     : Use message <N> (0 = 1500 'x' chars, 1 = 1500 '0123456789', 2 = long quote, 3 = "Msg0")
    -M <0|1>   : 0=disable 1=enable output of Messages (input and output) per time step
    -x <0|1>   : 0=disable 1=enable output of XMIT output per time step
    -r <0|1>   : 0=disable 1=enable output of RECV output per time step
```

## Design Details

We should really fill this in more; this is a sketch of the operations of the transmit
and receive pipelines (derived from GnuRadio's IEEE802.11 pipelines, implemented by
Bastian Bloessl ad et. al. (https://github.com/bastibl/gr-ieee802-11)


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

More development is needed and expected.

## Contacts and Current Maintainers

 - J-D Wellman (wellman@us.ibm.com)


