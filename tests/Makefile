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

INCLUDES=-g -I../include -fsanitize=address -fsanitize=leak

TESTS = test_read_yaml_ptt \
	test_getopt_extra \
	test_topology \
	test_results

BSTRLIB_OBJ := ../src/bstrlib.c ../src/bstrlib_helper.c
BSTRLIB_HEADER := ../include/bstrlib.h ../include/bstrlib_helper.h 
MAP_OBJ := ../src/map.c ../src/ghash.c
MAP_HEADER := ../include/map.h ../include/ghash.h
CALCULATOR_OBJ := ../src/calculator.c ../src/stack.c
CALCULATOR_HEADER := ../include/calculator.h ../include/stack.h
TOPOLOGY_OBJ := ../src/topology.c
TOPOLOGY_HEADER := ../include/topology.h
READ_YAML_OBJ := ../src/read_yaml_ptt.c
READ_YAML_HEADER := ../include/read_yaml_ptt.h

all: $(TESTS)

test_read_yaml_ptt: test_read_yaml_ptt.c $(READ_YAML_OBJ) $(READ_YAML_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER)
	$(CC) $(INCLUDES) -DWITH_BSTRING test_read_yaml_ptt.c $(READ_YAML_OBJ) $(BSTRLIB_OBJ) -o $@

test_results: test_results.c ../src/results.c $(BSTRLIB_OBJ) $(BSTRLIB_HEADER) $(MAP_OBJ) $(MAP_HEADER) $(CALCULATOR_OBJ) $(CALCULATOR_HEADER)
	$(CC) $(INCLUDES) -DWITH_BSTRING -DCALCULATOR_AS_LIB test_results.c ../src/results.c $(BSTRLIB_OBJ) $(MAP_OBJ) $(CALCULATOR_OBJ) -o $@ -lm


test_getopt_extra: test_getopt_extra.c ../src/getopt_extra.c $(BSTRLIB_OBJ) $(BSTRLIB_HEADER) $(MAP_OBJ) $(MAP_HEADER)
	$(CC) $(INCLUDES) -DWITH_BSTRING test_getopt_extra.c ../src/getopt_extra.c $(BSTRLIB_OBJ) $(MAP_OBJ) -o $@

test_topology: test_topology.c $(TOPOLOGY_OBJ) $(TOPOLOGY_HEADER) $(BSTRLIB_OBJ) $(BSTRLIB_HEADER)
	$(CC) $(INCLUDES) test_topology.c $(TOPOLOGY_OBJ) $(BSTRLIB_OBJ) -o $@

run: $(TESTS)
	@for T in $(TESTS); do ./$$T; done

clean:
	rm -f $(TESTS)
