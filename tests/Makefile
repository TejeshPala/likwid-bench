# =======================================================================================
#
#      Filename:  Makefile
#
#      Description:  Makefile for tests
#
#      Version:   <VERSION>
#      Released:  <DATE>
#
#      Author:  Thomas Gruber (tr), thomas.roehl@googlemail.com
#      Project:  LikwidBench
#
#      Copyright (C) 2016 Jan Treibig
#
#      This program is free software: you can redistribute it and/or modify it under
#      the terms of the GNU General Public License as published by the Free Software
#      Foundation, either version 3 of the License, or (at your option) any later
#      version.
#
#      This program is distributed in the hope that it will be useful, but WITHOUT ANY
#      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
#      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License along with
#      this program.  If not, see <http://www.gnu.org/licenses/>.
#
# =======================================================================================

CFLAGS = -g -fsanitize=address -fsanitize=leak
INCLUDES = -I../include

TESTS = test_read_yaml_ptt \
	test_topology \
	test_results \
	test_ptt2asm

# External stuff
BSTRLIB_OBJ := ../ext/bstrlib_helper/ext/bstrlib/bstrlib.c ../ext/bstrlib_helper/bstrlib_helper.c
BSTRLIB_HEADER := ../ext/bstrlib_helper/ext/bstrlib/bstrlib.h ../ext/bstrlib_helper/bstrlib_helper.h
INCLUDES += -I../ext/bstrlib_helper -I../ext/bstrlib_helper/ext/bstrlib

MAP_OBJ := ../ext/cmap/src/map.c ../ext/cmap/src/ghash.c
MAP_HEADER := ../ext/cmap/include/map.h ../ext/cmap/include/ghash.h
INCLUDES += -I../ext/cmap/include

CALCULATOR_OBJ := ../ext/calculator/calculator.c ../ext/calculator/stack.c
CALCULATOR_HEADER := ../ext/calculator/calculator.h ../ext/calculator/stack.h
INCLUDES += -I../ext/calculator

# Internal stuff
TOPOLOGY_OBJ := ../src/topology.c
TOPOLOGY_HEADER := ../include/topology.h

READ_YAML_OBJ := ../src/read_yaml_ptt.c
READ_YAML_HEADER := ../include/read_yaml_ptt.h

PTT2ASM_OBJ := ../src/ptt2asm.c
PTT2ASM_HEADER := ../include/ptt2asm.h ../include/isa_armv7.h ../include/isa_armv8.h ../include/isa_ppc64.h ../include/isa_x86-64.h ../include/isa_x86.h
PTT_KEYWORDS_HEADER := ../include/ptt_keyword_loop.h ../include/ptt_keyword_dummy.h

WORKGROUPS_OBJ := ../src/workgroups.c
WORKGROUPS_HEADER := ../include/workgroups.h

RESULTS_OBJ := ../src/results.c
RESULTS_HEADER := ../include/results.h

all: $(TESTS)

test_read_yaml_ptt: test_read_yaml_ptt.c $(READ_YAML_OBJ) $(READ_YAML_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER)
	$(CC) $(INCLUDES) $(CFLAGS) -DWITH_BSTRING test_read_yaml_ptt.c $(READ_YAML_OBJ) $(BSTRLIB_OBJ) -o $@

test_results: test_results.c ../src/results.c $(RESULTS_OBJ) $(RESULTS_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER) $(MAP_OBJ) $(MAP_HEADER) $(CALCULATOR_OBJ) $(CALCULATOR_HEADER)
	$(CC) $(INCLUDES) $(CFLAGS) -DWITH_BSTRING -DCALCULATOR_AS_LIB test_results.c $(RESULTS_OBJ) $(BSTRLIB_OBJ) $(MAP_OBJ) $(CALCULATOR_OBJ) -o $@ -lm


test_topology: test_topology.c $(TOPOLOGY_OBJ) $(TOPOLOGY_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER)
	$(CC) $(INCLUDES) $(CFLAGS) test_topology.c $(TOPOLOGY_OBJ) $(BSTRLIB_OBJ) -o $@

test_ptt2asm: test_ptt2asm.c $(PTT2ASM_OBJ) $(PTT2ASM_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER) $(PTT_KEYWORDS_HEADER)
	$(CC) $(INCLUDES) $(CFLAGS) test_ptt2asm.c $(PTT2ASM_OBJ) $(BSTRLIB_OBJ) -o $@

test_workgroups: test_workgroups.c $(RESULTS_OBJ) $(RESULTS_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER) $(TOPOLOGY_OBJ) $(TOPOLOGY_HEADER) $(WORKGROUPS_OBJ) $(WORKGROUPS_HEADER) $(MAP_OBJ) $(MAP_HEADER)
	$(CC) $(INCLUDES) $(CFLAGS) -DWITH_BSTRING test_workgroups.c $(TOPOLOGY_OBJ) $(BSTRLIB_OBJ) $(WORKGROUPS_OBJ) $(MAP_OBJ) $(RESULTS_OBJ) -o $@

run: $(TESTS)
	@for T in $(TESTS); do ./$$T; done

clean:
	rm -f $(TESTS)
