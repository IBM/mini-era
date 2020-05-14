// This code is intended to replicate the ERAv3 functionality
//  By translating the GnuRadio ieee802.11p pipeline from ERA to a c program
//  This includes both the transmit and receive functional pipelines.


// TRANSMIT:
//  1. Form a message : input string up to 1500 characters : char[1501] (terminating '\0')?
//  2. WiFi-MAC : gr-ieee802-11/mac.cc
//  3. Wifi-Mapper : gr-ieee802-11/mapper.cc
//  4. Packet-Header Generator : gnuradio/gr-digital/lib/packet_headergenerator_bb_impl.cc
//  5a. Chunks-to-Symbols : gnuradio/gr-digital/lib/chunks_to_symbols_impl.*
//  5b. Chunks-to-Symbols : gr-ieee802-11/chunks_to_symbols_impl.cc
//  6. Tagged-Stream-Mux : gnuradio/gr-blocks/lib/tagged_stream_mux_impl.cc  :: <concatenate: header from 5a and data symbols from 5b> :
//  7. OFDM-Carrier-Allocator : gnuradio/gr-digital/lib/ofdm_carrier_allocator_cvc_impl.cc
//  8. iFFT : gnuradio/gr-fft (uses FFTW lib) : 64-size Inverse FFT
//  9. OFDM-Cycle-Prefixer : gnuradio/gr-digital/lib/ofdm_cyclic_prefixer_impl.cc 
//  X. OUTPUT : <some number of complex numbers?>

// RECEIVE:
//  1. Delay is just a samples delay function (shift register)
//  2. Complex Conjugate (volk) : a+ib -> a-ib  (i.e. reverse sign of imaginary part)
//  3. Complex to Mag^2 : a+ib -> (a^2 + b^2)  
//  4. Multiply : Complex Multiply
//  5. Moving Average :  gr-ieee802-11/moving_average_XX_impl.cc.t 
//  6. Complex to Mag   : a+ib -> SQRT(a^2 + b^2)  
//  7. Divide : FP Divide
//  8. WIFI-Sync-Short : gr-ieee802-11/sync_short.cc
//  9. WIFI-Sync-Long : gr-ieee802-11/sync_long.cc
// 10. Stream-to-Vector : gnuradio/gr-blocks/lib/streams_to_vector_impl.cc 
// 11. FFT : gnuradio/gr-fft (uses FFTW lib) : 64-size FFT
// 12. WIFI-Frame-Equalizer : gr-ieee802-11/frame_equalizer_impl.cc
// 13. WIFI-Decode-MAC : gr-ieee802-11/decode_mac.cc
//  X. OUTPUT : the message payload? (1500 symbols)



