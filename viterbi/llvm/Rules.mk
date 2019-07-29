all: needle-run-path 

setup: .setup.done
.setup.done: 
	@echo "SETUP"
	@rm -rf $(FUNCTION)
	@mkdir $(FUNCTION)
	$(LLVM_OBJ)/llvm-link $(BC) $(BITCODE_REPO)/$(LLVM_VERSION)/lib/m.bc -o $(FUNCTION)/$(NAME).bc
	$(LLVM_OBJ)/opt -O2 -disable-loop-vectorization -disable-slp-vectorization  $(FUNCTION)/$(NAME).bc -o $(FUNCTION)/$(NAME).bc
	@touch .setup.done

epp-inst: .setup.done .epp-inst.done
.epp-inst.done: .setup.done 
	@echo "EPP-INST"
	cd $(FUNCTION) && \
	export PATH=$(LLVM_OBJ):$(PATH) && \
		$(NEEDLE_OBJ)/epp $(LDFLAGS) -L$(NEEDLE_LIB) -epp-fn=$(FUNCTION) $(NAME).bc -o $(NAME)-epp $(LIBS) 2> ../epp-inst.log
	@touch .epp-inst.done

prerun: .setup.done 
.prerun.done: .setup.done 
ifdef PRERUN
	cd $(FUNCTION) && \
	bash -c $(PRERUN)
endif
	@touch .prerun.done

rle-pack: .epp-inst.done .epp-run.done .prerun.done .rle-pack.done
.rle-pack.done: .epp-run.done
	@echo "RLE Pack to Gzip"
	cd $(FUNCTION) && \
		nl -baln < path-profile-trace.txt | gzip -c > $(NAME).gz


epp-run: .epp-inst.done .epp-run.done .prerun.done
.epp-run.done: .epp-inst.done .prerun.done
	@echo "EPP-RUN"
	cd $(FUNCTION) && \
	export LD_LIBRARY_PATH=$(NEEDLE_LIB):/usr/local/lib64 && \
	./$(NAME)-epp $(RUNCMD) 2>&1 > ../epp-run.log
	@touch .epp-run.done

epp-decode: .epp-run.done .epp-decode.done
.epp-decode.done: .epp-run.done 
	@echo "EPP-DECODE"
	cd $(FUNCTION) && \
	export PATH=$(LLVM_OBJ):$(PATH) && \
    $(NEEDLE_OBJ)/epp -epp-fn=$(FUNCTION) $(NAME).bc -p=path-profile-results.txt 2> ../epp-decode.log
	@touch .epp-decode.done

needle-path: .epp-decode.done .needle-path.done
.needle-path.done: .epp-decode.done 
	@echo "NEEDLE-PATH"
	cd $(FUNCTION) && \
	export PATH=$(LLVM_OBJ):$(PATH) && \
    python $(ROOT)/examples/scripts/path.py epp-sequences.txt > paths.stats.txt && \
	$(NEEDLE_OBJ)/needle -fn=$(FUNCTION) -ExtractType::path -slog -seq=path-seq-0.txt $(LIBS) -u=$(HELPER_LIB) $(NAME).bc -o $(NAME)-needle-0 2>&1 > ../needle-path.log

needle-braid: .epp-decode.done .needle-braid.done
.needle-braid.done: .epp-decode.done 
	@echo "NEEDLE-BRAID"
	cd $(FUNCTION) && \
	export PATH=$(LLVM_OBJ):$(PATH) && \
    python $(ROOT)/examples/scripts/braid.py epp-sequences.txt > braids.stats.txt && \
	$(NEEDLE_OBJ)/needle -fn=$(FUNCTION) -ExtractType::braid -seq=braid-seq-0.txt $(LIBS) -u=$(HELPER_LIB) $(NAME).bc -o $(NAME)-needle-0 2>&1 > ../needle-braid.log

needle-run-braid: needle-braid prerun .prerun.done
	@echo NEEDLE-RUN-BRAID
	cd $(FUNCTION) && \
	export LD_LIBRARY_PATH=$(NEEDLE_LIB):/usr/local/lib64 && \
	./$(NAME)-needle-0 $(RUNCMD) 2>&1 > ../needle-run-braid.log

needle-run-path: needle-path prerun .prerun.done
	@echo NEEDLE-RUN-PATH
	cd $(FUNCTION) && \
	export LD_LIBRARY_PATH=$(NEEDLE_LIB):/usr/local/lib64 && \
	./$(NAME)-needle-0 $(RUNCMD) 2>&1 > ../needle-run-path.log

clean:
	@rm -rf $(FUNCTION) *.log .*.done *.bc

.PHONY: clean setup
