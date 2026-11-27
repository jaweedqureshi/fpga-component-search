VERILATOR ?= verilator
TARGET := build/obj_dir/Vrecord_comparator
RTL := rtl/record_comparator.sv
TESTBENCH := sim/tb_phase1.cpp

.PHONY: all build test clean

all: test

build: $(TARGET)

$(TARGET): $(RTL) $(TESTBENCH)
	mkdir -p build
	$(VERILATOR) --cc --exe --build --x-assign fast --x-initial unique -Wall \
		-Wno-DECLFILENAME -O3 --Mdir build/obj_dir \
		--top-module record_comparator $(abspath $(RTL)) $(abspath $(TESTBENCH))

test: build
	./$(TARGET)

clean:
	rm -rf build/obj_dir
