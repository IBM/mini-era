CC     = gcc -std=c99 -O2
CFLAGS = -g

AR      = ar rcs

OBJDIR = obj
OBJDIR_E = obj_esp
OBJDIR_G = obj_gp
OBJDIR_T = obj_t
OBJDIR_CK = obj_ck
OBJDIR_ECK = obj_esp_ck
OBJDIR_GCK = obj_gp_ck
OBJDIR_TCK = obj_t_ck
OBJDIR_EGCK = obj_esp_gp_ck
OBJDIR_ETCK = obj_esp_t_ck

OBJDIR_F = obj_f
OBJDIR_FE = obj_f_esp
OBJDIR_FG = obj_f_gp
OBJDIR_FT = obj_f_t
OBJDIR_FV = obj_fv
OBJDIR_FVE = obj_fv_esp
OBJDIR_FVG = obj_fv_gp
OBJDIR_FVT = obj_fv_t
OBJDIR_FVEG = obj_fv_esp_gp
OBJDIR_FVET = obj_fv_esp_t
OBJDIR_FEG = obj_f_esp_gp
OBJDIR_FET = obj_f_esp_t
OBJDIR_FCK = obj_fck

L_SRC = viterbi_standalone.c viterbi_decoder_generic.c descrambler_function.c
L_OBJ = $(L_SRC:%.c=$(OBJDIR)/%.o)
L_OBJ_E = $(L_SRC:%.c=$(OBJDIR_E)/%.o)
L_OBJ_G = $(L_SRC:%.c=$(OBJDIR_G)/%.o)
L_OBJ_T = $(L_SRC:%.c=$(OBJDIR_T)/%.o)

LF_SRC = viterbi_standalone.c viterbi_flat.c descrambler_function.c
LF_OBJ = $(LF_SRC:%.c=$(OBJDIR_F)/%.o)
LF_OBJ_E = $(LF_SRC:%.c=$(OBJDIR_FE)/%.o)
LF_OBJ_G = $(LF_SRC:%.c=$(OBJDIR_FG)/%.o)
LF_OBJ_T = $(LF_SRC:%.c=$(OBJDIR_FT)/%.o)

LFV_SRC = viterbi_standalone.c viterbi_flat.c descrambler_function.c
LFV_OBJ = $(LFV_SRC:%.c=$(OBJDIR_FV)/%.o)
LFV_OBJ_E = $(LFV_SRC:%.c=$(OBJDIR_FVE)/%.o)
LFV_OBJ_G = $(LFV_SRC:%.c=$(OBJDIR_FVG)/%.o)
LFV_OBJ_T = $(LFV_SRC:%.c=$(OBJDIR_FVT)/%.o)

LCK_OBJ = $(L_SRC:%.c=$(OBJDIR_CK)/%.o)
LCK_OBJ_E = $(L_SRC:%.c=$(OBJDIR_ECK)/%.o)
LCK_OBJ_G = $(L_SRC:%.c=$(OBJDIR_GCK)/%.o)
LCK_OBJ_T = $(L_SRC:%.c=$(OBJDIR_TCK)/%.o)

#A_SRC = api_test.c
#A_OBJ = $(A_SRC:%.c=obj/%.o)


T_SRC   = vtest.c
T_OBJ   = $(T_SRC:%.c=obj/%.o)
T_OBJ_E = $(T_SRC:%.c=$(OBJDIR_E)/%.o)
T_OBJ_G = $(T_SRC:%.c=$(OBJDIR_G)/%.o)
T_OBJ_T = $(T_SRC:%.c=$(OBJDIR_T)/%.o)

VT_SRC   = viterbi_test.c
VT_OBJ   = $(VT_SRC:%.c=obj/%.o)
VT_OBJ_E = $(VT_SRC:%.c=$(OBJDIR_E)/%.o)
VT_OBJ_G = $(VT_SRC:%.c=$(OBJDIR_G)/%.o)
VT_OBJ_T = $(VT_SRC:%.c=$(OBJDIR_T)/%.o)

FT_SRC   = viterbi_test.c
FT_OBJ   = $(FT_SRC:%.c=$(OBJDIR_F)/%.o)
FT_OBJ_E = $(FT_SRC:%.c=$(OBJDIR_FE)/%.o)
FT_OBJ_G = $(FT_SRC:%.c=$(OBJDIR_FG)/%.o)
FT_OBJ_T = $(FT_SRC:%.c=$(OBJDIR_FT)/%.o)

FTV_SRC = viterbi_test.c
FTV_OBJ = $(FTV_SRC:%.c=$(OBJDIR_FV)/%.o)
FTV_OBJ_E = $(FTV_SRC:%.c=$(OBJDIR_FVE)/%.o)
FTV_OBJ_G = $(FTV_SRC:%.c=$(OBJDIR_FVG)/%.o)
FTV_OBJ_T = $(FTV_SRC:%.c=$(OBJDIR_FVT)/%.o)

all:	lib vtest vtest_esp vtest_gp vtest_t viterbi_test viterbi_test_esp viterbi_test_gp viterbi_test_t viterbi_ftest viterbi_ftest_esp viterbi_ftest_gp viterbi_ftest_t viterbi_fvtest viterbi_fvtest_esp viterbi_fvtest_gp viterbi_fvtest_t
	echo Done building all

lib:	libviterbi.a libviterbi_esp.a libviterbi_gp.a libviterbi_t.a libviterbiCk.a libviterbiCk_esp.a libviterbiCk_gp.a libviterbiCk_t.a libviterbiF.a libviterbiF_esp.a libviterbiF_gp.a libviterbiF_t.a libviterbiFV.a libviterbiFV_esp.a libviterbiFV_gp.a libviterbiFV_t.a
	echo Built $@


libviterbi.a: $(OBJDIR) $(L_OBJ)
	$(AR) $@ $(L_OBJ)

libviterbi_esp.a: $(OBJDIR_E) $(L_OBJ_E)
	$(AR) $@ $(L_OBJ_E)

libviterbi_gp.a: $(OBJDIR_G) $(L_OBJ_G)
	$(AR) $@ $(L_OBJ_G)

libviterbi_t.a: $(OBJDIR_T) $(L_OBJ_T)
	$(AR) $@ $(L_OBJ_T)

libviterbiCk.a: $(OBJDIR_CK) $(LCK_OBJ)
	$(AR) $@ $(LCK_OBJ)

libviterbiCk_esp.a: $(OBJDIR_ECK) $(LCK_OBJ_E)
	$(AR) $@ $(LCK_OBJ_E)

libviterbiCk_gp.a: $(OBJDIR_GCK) $(LCK_OBJ_G)
	$(AR) $@ $(LCK_OBJ_G)

libviterbiCk_t.a: $(OBJDIR_TCK) $(LCK_OBJ_T)
	$(AR) $@ $(LCK_OBJ_T)

libviterbiF.a: $(OBJDIR_F) $(LF_OBJ)
	$(AR) $@ $(LF_OBJ)

libviterbiF_esp.a: $(OBJDIR_FE) $(LF_OBJ_E)
	$(AR) $@ $(LF_OBJ_E)

libviterbiF_gp.a: $(OBJDIR_FG) $(LF_OBJ_G)
	$(AR) $@ $(LF_OBJ_G)

libviterbiF_t.a: $(OBJDIR_FT) $(LF_OBJ_T)
	$(AR) $@ $(LF_OBJ_T)

libviterbiFV.a: $(OBJDIR_FV) $(LFV_OBJ)
	$(AR) $@ $(LFV_OBJ)

libviterbiFV_esp.a: $(OBJDIR_FVE) $(LFV_OBJ_E)
	$(AR) $@ $(LFV_OBJ_E)

libviterbiFV_gp.a: $(OBJDIR_FVG) $(LFV_OBJ_G)
	$(AR) $@ $(LFV_OBJ_G)

libviterbiFV_t.a: $(OBJDIR_FVT) $(LFV_OBJ_T)
	$(AR) $@ $(LFV_OBJ_T)




api_test: $(A_OBJ) libviterbi.a
	$(CC) $(A_OBJ) -L . -lviterbi -o $@

vtest: $(T_OBJ) libviterbiCk.a
	$(CC) $(T_OBJ) -L . -lviterbiCk -o $@

vtest_esp: $(T_OBJ_E) libviterbiCk_esp.a
	$(CC) $(T_OBJ_E) -L . -lviterbiCk_esp -o $@

vtest_gp: $(T_OBJ_G) libviterbiCk_gp.a
	$(CC) $(T_OBJ_G) -L . -lviterbiCk_gp -o $@

vtest_t: $(T_OBJ_T) libviterbiCk_t.a
	$(CC) $(T_OBJ_T) -L . -lviterbiCk_t -o $@

viterbi_test: $(VT_OBJ) libviterbi.a
	$(CC) $(VT_OBJ) -L . -lviterbi -o $@

viterbi_test_esp: $(VT_OBJ_E) libviterbi_esp.a
	$(CC) $(VT_OBJ_E) -L . -lviterbi_esp -o $@

viterbi_test_gp: $(VT_OBJ_G) libviterbi_gp.a
	$(CC) $(VT_OBJ_G) -L . -lviterbi_gp -o $@

viterbi_test_t: $(VT_OBJ_T) libviterbi_t.a
	$(CC) $(VT_OBJ_T) -L . -lviterbi_t -o $@

viterbi_ftest: $(FT_OBJ) libviterbiF.a
	$(CC) $(FT_OBJ) -L . -lviterbiF -o $@

viterbi_ftest_esp: $(FT_OBJ_E) libviterbiF_esp.a
	$(CC) $(FT_OBJ_E) -L . -lviterbiF_esp -o $@

viterbi_ftest_gp: $(FT_OBJ_G) libviterbiF_gp.a
	$(CC) $(FT_OBJ_G) -L . -lviterbiF_gp -o $@

viterbi_ftest_t: $(FT_OBJ_T) libviterbiF_t.a
	$(CC) $(FT_OBJ_T) -L . -lviterbiF_t -o $@

viterbi_fvtest: $(FTV_OBJ) libviterbiFV.a
	$(CC) $(FTV_OBJ) -L . -lviterbiFV -o $@

viterbi_fvtest_esp: $(FTV_OBJ_E) libviterbiFV_esp.a
	$(CC) $(FTV_OBJ_E) -L . -lviterbiFV_esp -o $@

viterbi_fvtest_gp: $(FTV_OBJ_G) libviterbiFV_gp.a
	$(CC) $(FTV_OBJ_G) -L . -lviterbiFV_gp -o $@

viterbi_fvtest_t: $(FTV_OBJ_T) libviterbiFV_t.a
	$(CC) $(FTV_OBJ_T) -L . -lviterbiFV_t -o $@


$(OBJDIR):
	mkdir $@

$(OBJDIR_E):
	mkdir $@

$(OBJDIR_G):
	mkdir $@

$(OBJDIR_T):
	mkdir $@

$(OBJDIR_CK):
	mkdir $@

$(OBJDIR_ECK):
	mkdir $@

$(OBJDIR_GCK):
	mkdir $@

$(OBJDIR_TCK):
	mkdir $@

$(OBJDIR_EGCK):
	mkdir $@

$(OBJDIR_ETCK):
	mkdir $@


$(OBJDIR_F):
	mkdir $@

$(OBJDIR_FE):
	mkdir $@

$(OBJDIR_FG):
	mkdir $@

$(OBJDIR_FT):
	mkdir $@

$(OBJDIR_FV):
	mkdir $@

$(OBJDIR_FVE):
	mkdir $@

$(OBJDIR_FVEG):
	mkdir $@

$(OBJDIR_FVG):
	mkdir $@

$(OBJDIR_FEG):
	mkdir $@

$(OBJDIR_FVET):
	mkdir $@

$(OBJDIR_FVT):
	mkdir $@

$(OBJDIR_FET):
	mkdir $@

$(OBJDIR_FCK):
	mkdir $@


clean:
	-rm libviterbi.a $(L_OBJ)
	-rm libviterbi_esp.a $(L_OBJ_E)
	-rm libviterbi_gp.a $(L_OBJ_G)
	-rm libviterbi_t.a $(L_OBJ_T)
	-rm libviterbiCk.a $(LCK_OBJ)
	-rm libviterbiCk_esp.a $(LCK_OBJ_E)
	-rm libviterbiCk_gp.a $(LCK_OBJ_G)
	-rm libviterbiCk_t.a $(LCK_OBJ_T)
	-rm libviterbiF.a $(LF_OBJ)
	-rm libviterbiF_esp.a $(LF_OBJ_E)
	-rm libviterbiF_gp.a $(LF_OBJ_G)
	-rm libviterbiF_t.a $(LF_OBJ_T)
	-rm libviterbiFV.a $(LFV_OBJ)
	-rm libviterbiFV_esp.a $(LFV_OBJ_E)
	-rm libviterbiFV_gp.a $(LFV_OBJ_G)
	-rm libviterbiFV_t.a $(LFV_OBJ_T)
	-rm api_test $(A_OBJ)
	-rm vtest $(T_OBJ)
	-rm vtest_esp $(T_OBJ_E)
	-rm vtest_gp $(T_OBJ_G)
	-rm vtest_t $(T_OBJ_T)
	-rm viterbi_test $(VT_OBJ)
	-rm viterbi_test_esp $(VT_OBJ_E)
	-rm viterbi_test_gp $(VT_OBJ_G)
	-rm viterbi_test_t $(VT_OBJ_T)
	-rm viterbi_ftest $(FT_OBJ)
	-rm viterbi_ftest_esp $(FT_OBJ_E)
	-rm viterbi_ftest_gp $(FT_OBJ_G)
	-rm viterbi_ftest_t $(FT_OBJ_T)
	-rm viterbi_fvtest $(FTV_OBJ)
	-rm viterbi_fvtest_esp $(FTV_OBJ_E)
	-rm viterbi_fvtest_gp $(FTV_$BJ_G)
	-rm viterbi_fvtest_t $(FTV_$BJ_T)

allclean: clean
	$(RM) -r $(OBJDIR)
	$(RM) -r $(OBJDIR_E)
	$(RM) -r $(OBJDIR_G)
	$(RM) -r $(OBJDIR_CK)
	$(RM) -r $(OBJDIR_ECK)
	$(RM) -r $(OBJDIR_GCK)
	$(RM) -r $(OBJDIR_EGCK)
	$(RM) -r $(OBJDIR_F)
	$(RM) -r $(OBJDIR_FE)
	$(RM) -r $(OBJDIR_FG)
	$(RM) -r $(OBJDIR_FV)
	$(RM) -r $(OBJDIR_FVE)
	$(RM) -r $(OBJDIR_FVG)
	$(RM) -r $(OBJDIR_FVEG)
	$(RM) -r $(OBJDIR_FEG)
	$(RM) -r $(OBJDIR_FCK)


obj/%.o: %.c
	$(CC) $*.c $(CFLAGS) -c -o $@

$(OBJDIR_E)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -c -o $@

$(OBJDIR_G)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -g -pg -c -o $@

$(OBJDIR_EG)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -g -pg -c -o $@

$(OBJDIR_T)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DINT_TIME -c -o $@

$(OBJDIR_ET)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -DINT_TIME -c -o $@

$(OBJDIR_CK)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DDO_RUN_TIME_CHECKING -c -o $@

$(OBJDIR_ECK)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -c -o $@

$(OBJDIR_GCK)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -g -pg -c -o $@

$(OBJDIR_EGCK)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -g -pg -c -o $@

$(OBJDIR_TCK)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DINT_TIME -c -o $@

$(OBJDIR_ETCK)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -DINT_TIME -c -o $@

$(OBJDIR_F)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -c -o $@

$(OBJDIR_FE)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -c -o $@

$(OBJDIR_FG)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -g -pg -c -o $@

$(OBJDIR_FEG)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -g -pg -c -o $@

$(OBJDIR_FT)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DINT_TIME -c -o $@

$(OBJDIR_FET)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DUSE_ESP_INTERFACE -DINT_TIME -c -o $@

$(OBJDIR_FV)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DVERBOSE_OUTPUT -c -o $@

$(OBJDIR_FVE)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DVERBOSE_OUTPUT -DUSE_ESP_INTERFACE -c -o $@

$(OBJDIR_FVG)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DVERBOSE_OUTPUT -g -pg -c -o $@

$(OBJDIR_FVEG)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DVERBOSE_OUTPUT -DUSE_ESP_INTERFACE -g -pg -c -o $@

$(OBJDIR_FVT)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DVERBOSE_OUTPUT -DINT_TIME -c -o $@

$(OBJDIR_FVET)/%.o: %.c
	$(CC) $*.c $(CFLAGS) -DVERBOSE_OUTPUT -DUSE_ESP_INTERFACE -DINT_TIME -c -o $@



$(OBJDIR)/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR)/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR)/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR)/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR)/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR)/atest.o: viterbi_standalone.h utils.h

$(OBJDIR_CK)/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR_CK)/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR_CK)/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR_F)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_F)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_F)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_F)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FV)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FV)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FV)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FV)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_E)/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_E)/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_E)/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR_E)/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR_E)/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR_CK)_esp/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)_esp/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)_esp/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR_CK)_esp/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR_CK)_esp/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR_F)_esp/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_F)_esp/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_F)_esp/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_F)_esp/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FVE)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVE)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVE)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FVE)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_G)/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_G)/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_G)/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR_G)/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR_G)/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR_CK)_gp/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)_gp/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)_gp/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR_CK)_gp/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR_CK)_gp/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR_F)_gp/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_F)_gp/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_F)_gp/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_F)_gp/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FVG)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVG)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVG)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FVG)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_CK)_t/viterbi_standalone.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)_t/viterbi_decoder_generic.o: viterbi_decoder_generic.h base.h utils.h
$(OBJDIR_CK)_t/viterbi_decoder_generic.o: viterbi_standalone.h
$(OBJDIR_CK)_t/vtest.o: viterbi_standalone.h utils.h
$(OBJDIR_CK)_t/viterbi_test.o: viterbi_standalone.h utils.h

$(OBJDIR_FT)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FT)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FT)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FT)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FVT)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVT)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVT)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FVT)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FVEG)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVEG)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVEG)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FVEG)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FVET)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVET)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FVET)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FVET)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FEG)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FEG)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FEG)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FEG)/viterbi_ftest.o: viterbi_standalone.h utils.h

$(OBJDIR_FET)/viterbi_standalone.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FET)/viterbi_flat.o: viterbi_flat.h base.h utils.h
$(OBJDIR_FET)/viterbi_flat.o: viterbi_standalone.h
$(OBJDIR_FET)/viterbi_ftest.o: viterbi_standalone.h utils.h


depend:;	makedepend -fMakefile -- $(CFLAGS) -- $(L_SRC) $(T_SRC)
# DO NOT DELETE THIS LINE -- make depend depends on it.

