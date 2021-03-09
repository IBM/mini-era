#ifndef INCLUDED_VITERBI_GENERIC_H
#define INCLUDED_VITERBI_GENERIC_H

/*
 * Copyright (C) 2016 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "base.h"


/* This Viterbi decoder was taken from the gr-dvbt module of
 * GNU Radio. It is an SSE2 version of the Viterbi Decoder
 * created by Phil Karn. The SSE2 version was made by Bogdan
 * Diaconescu. For more info see: gr-dvbt/lib/d_viterbi.h
 */
void viterbi_decode(ofdm_param *ofdm,   size_t ofdm_size,
		    frame_param *frame, size_t frame_size,
		    uint8_t *in,        size_t in_size,
		    uint8_t* l_decoded, size_t decd_size);

typedef union branchtab27 {
  unsigned char c[32];
} d_branchtab27_t;

void viterbi_chunks_init_generic(void);

void viterbi_butterfly2_generic(unsigned char *symbols,        size_t size_symbols,
				unsigned char *d_brtab27[2],   size_t size_brtab27,
				unsigned char *mm0,            size_t size_mm0,
				unsigned char *mm1,            size_t size_mm1,
				unsigned char *pp0,            size_t size_pp0,
				unsigned char *pp1,            size_t size_pp1 );

void viterbi_get_output_generic(unsigned char *mm0,                                    size_t mm0_size,
				unsigned char *pp0,                                    size_t pp0_size,
				int ntraceback,
				int* store_pos,                                        size_t size_store_pos,
				unsigned char*  mmresult __attribute__((aligned(16))), size_t mmres_size,
				unsigned char ppresult[TRACEBACK_MAX][64] __attribute__((aligned(16))), size_t ppres_size,
				unsigned char *outbuf,                                 size_t outbuf_size);


#endif
