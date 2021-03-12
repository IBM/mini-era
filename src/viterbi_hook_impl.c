// Prerequisites:
//    git clone https://github.com/sld-columbia/esp.git
//    git checkout epochs
// clang -S -O0  -emit-llvm vitebi_hook_impl.c -I /home/sdasgup3/Github/esp/soft/ariane/drivers/include/  -o vitebi_hook_impl.ll

#include "libesp.h"
#include "vitdodec_stratus.h"

typedef int8_t token_t;

/* <<--params-def-->> */
#define CBPS 48
#define NTRACEBACK 5
#define DATA_BITS 288

/* <<--params-->> */
const int32_t cbps = CBPS;
const int32_t ntraceback = NTRACEBACK;
const int32_t data_bits = DATA_BITS;

#define NACC 1



static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 24852;
		out_words_adj = 18585;
	} else {
		in_words_adj = round_up(24852, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(18585, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len =  out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
}


void do_decoding_hook(int in_n_data_bits, int in_cbps, int in_ntraceback, unsigned char *inMemory, unsigned char *outMemory)
{

	int errors;

	token_t *buf;

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	
	for (int i = 0; i < in_len; ++i) {
		buf[i] = inMemory[i];
	}

	for (int i = in_len; i < in_len+out_len; ++i) {
		buf[i] = 0;
	}

	struct vitdodec_stratus_access vitdodec_cfg_000[] = {
	    {
		/* <<--descriptor-->> */
		.cbps = in_cbps,
		.ntraceback = in_ntraceback,
		.data_bits = in_n_data_bits,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_NONE,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	    }
	};
	esp_thread_info_t cfg_000[] = {
	     {
		.run = true,
		.devname = "vitdodec_stratus.0",
		.ioctl_req = VITDODEC_STRATUS_IOC_ACCESS,
		.esp_desc = &(vitdodec_cfg_000[0].esp),
		.hw_buf = buf,
	     }
	};

	esp_run(cfg_000, NACC);

	for (int i = 0; i < out_len; ++i) {
		outMemory[i] = buf[i+in_len];
	}

	esp_free(buf);
}
