#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

###############################################################################
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

HELPER:= $(HELPER_CUR_DIR)/.HELPER

ifeq ($(DATA_TYPE), cint32)
	SAMPLES_IN_256_VECT 	  := 4
else ifeq ($(DATA_TYPE), cfloat)
	SAMPLES_IN_256_VECT 	  := 4
else ifeq ($(DATA_TYPE), cint16)
	SAMPLES_IN_256_VECT 	  := 8
else
	SAMPLES_IN_256_VECT 	  := 8
endif

PAD_POINT_SIZE 		:=  $(shell echo $$((   (($(POINT_SIZE) + $(SAMPLES_IN_256_VECT) - 1) / $(SAMPLES_IN_256_VECT)) * $(SAMPLES_IN_256_VECT) )))
WINDOW_VSIZE 	:= $(shell echo $$(( 	$(PAD_POINT_SIZE) * $(NUM_FRAMES)		)))
PARAM_MAP = DATA_TYPE $(DATA_TYPE) TWIDDLE_TYPE $(TWIDDLE_TYPE) POINT_SIZE $(POINT_SIZE) CASC_LEN $(CASC_LEN) NUM_FRAMES $(NUM_FRAMES) FFT_NIFFT $(FFT_NIFFT) SHIFT $(SHIFT) API_IO $(API_IO) AIE_VARIANT $(AIE_VARIANT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)
UUT_FILE_SUFFIX = $(UUT_KERNEL)_$(DATA_TYPE)_$(TWIDDLE_TYPE)_$(POINT_SIZE)_$(FFT_NIFFT)_$(SHIFT)_$(CASC_LEN)_$(NUM_FRAMES)_$(API_IO)_$(ROUND_MODE)_$(SAT_MODE)
LOG_FILE = ./logs/log_$(UUT_FILE_SUFFIX).txt
STATUS_LOG_FILE = ./logs/status_$(UUT_FILE_SUFFIX).txt
STATUS_FILE = $(STATUS_LOG_FILE)


$(HELPER): create_config validate_config create_input sim_ref prep_x86_out
	make cleanall

create_config:
	echo validating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

validate_config:
	vitis --classic -exec ipmetadata_config_checker $(HELPER_ROOT_DIR)/L2/meta/dft.json ./config.json -newflow


create_input:
	@echo starting create_input
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(REF_INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) 0 0 $(DATA_TYPE) $(API_IO) 1 0;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/input_pad_and_split.pl --variant $(AIE_VARIANT) --file $(REF_INPUT_FILE) --newFile $(LOC_INPUT_FILE) --type $(DATA_TYPE) --pointSize $(POINT_SIZE) --cascLen $(CASC_LEN) --numFrames $(NUM_FRAMES);\
	echo Input ready

sim_ref:
	make UUT_KERNEL=dft_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

check_op_ref: prep_x86_out prep_aie_out
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr 1 --zip --dual 0 -k 0 -w ${WINDOW_VSIZE};\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr 1 --zip --dual 0 -k 0 -w ${WINDOW_VSIZE};\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) PERCENT

get_status: check_op_ref
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_qor:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output dftMain $(NITER)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_fft_theoretical_min.tcl $(DATA_TYPE) $(TWIDDLE_TYPE) $(WINDOW_VSIZE) $(POINT_SIZE) $(CASC_LEN) $(STATUS_FILE) $(UUT_KERNEL) 0 $(API_IO)
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
