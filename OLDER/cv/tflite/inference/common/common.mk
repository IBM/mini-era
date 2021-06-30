# Copyright (c) 2011-2020 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

#### You may not need to edit the remaining of this Makefile. ####

ifndef V
	QUIET_AR            = @echo 'MAKE:' AR $@;
	QUIET_BUILD         = @echo 'MAKE:' BUILD $@;
	QUIET_C             = @echo 'MAKE:' CC $@;
	QUIET_CXX           = @echo 'MAKE:' CXX $@;
	QUIET_CHECKPATCH    = @echo 'MAKE:' CHECKPATCH $(subst .o,.cc,$@);
	QUIET_CHECK         = @echo 'MAKE:' CHECK $(subst .o,.cc,$@);
	QUIET_LINK          = @echo 'MAKE:' LINK $@;
	QUIET_CP            = @echo 'MAKE:' CP $@;
	QUIET_MKDIR         = @echo 'MAKE:' MKDIR $@;
	QUIET_MAKE          = @echo 'MAKE:' MAKE $@;
	QUIET_INFO          = @echo -n 'MAKE:' INFO '';
	QUIET_RUN           = @echo 'MAKE:' RUN '';
	QUIET_CLEAN         = @echo 'MAKE:' CLEAN ${PWD};
endif

all: compact
.PHONY: all

THIRD_PARTY := ../../../third_party
TENSORFLOW_DIR := $(THIRD_PARTY)/tensorflow

TARGET ?= linux

include $(wildcard ./targets/*_makefile.inc)

LIBS += $(THIRD_PARTY)/libs/$(TARGET)_$(TARGET_ARCH)/libtensorflow-lite.a
LIBS += -lpthread

CXX := $(TARGET_TOOLCHAIN_PREFIX)g++
CC := $(TARGET_TOOLCHAIN_PREFIX)gcc
AR := $(TARGET_TOOLCHAIN_PREFIX)ar

INCDIR += -I.
INCDIR += -I./behav
INCDIR += -I$(TENSORFLOW_DIR)
INCDIR += -I$(TENSORFLOW_DIR)/tensorflow/lite/tools/make/downloads/flatbuffers/include
CXX_FLAGS += -MMD
CXX_FLAGS += -std=c++14

#release: CXX_FLAGS += -O3
#release: CXX_FLAGS += -DUSE_DATA_READER
#release: $(MODEL)
#.PHONY: realease
#
#release-gui: CXX_FLAGS += -DUSE_OPENCV
#release-gui: LD_FLAGS += -lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core
#release-gui: $(MODEL)
#.PHONY: release-gui

compact: CXX_FLAGS += -O3
compact: $(MODEL)
.PHONY: compact

compact-gui: CXX_FLAGS += -DUSE_OPENCV
compact-gui: LD_FLAGS += -lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core
compact-gui: $(MODEL)
.PHONY: compact-gui

debug: CXX_FLAGS += -O0
debug: CXX_FLAGS += -g
debug: $(MODEL)
	$(QUIET_INFO)echo "Compiled with debugging flags!"
.PHONY: debug

VPATH += .

CXX_SOURCES += $(wildcard ./*.cc)

.SUFFIXES: .cc .h .o .hpp

CXX_OBJECTS := $(CXX_SOURCES:.cc=.o)
-include $(CXX_OBJECTS:.o=.d)

$(MODEL): $(CXX_OBJECTS)
	$(QUIET_LINK)$(CXX) -o $@ $(CXX_OBJECTS) ${LIBS} ${LD_FLAGS}

.cc.o:
	$(QUIET_CXX)$(CXX) $(CXX_FLAGS) ${INCDIR} -c $<

run-compact: compact
	$(QUIET_RUN)./$(MODEL) -m $(TFLITE_FILE) -v 1
.PHONY: run-compact

#run-release: release
#	$(QUIET_RUN)./$(MODEL) -m $(TFLITE_FILE) -v 1 $(TF_FLAGS)
#.PHONY: run-release

valgrind: debug
	$(QUIET_RUN)valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(MODEL)
.PHONY: valgrind

upload:
	../../../common/upload.sh $(MODEL) $(TFLITE_FILE)
.PHONY: upload

gdb:
	$(QUIET_RUN)gdb ./$(MODEL)
.PHONY: gdb

clean:
	$(QUIET_CLEAN)rm -rf *.o *.d $(MODEL)
.PHONY: clean
