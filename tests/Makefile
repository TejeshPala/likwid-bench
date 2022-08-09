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

INCLUDES=-I../include -fsanitize=address -fsanitize=leak

test_read_yaml_ptt: test_read_yaml_ptt.c ../src/read_yaml_ptt.c ../src/bstrlib.c ../src/bstrlib_helper.c
	$(CC) $(INCLUDES) test_read_yaml_ptt.c ../src/read_yaml_ptt.c ../src/bstrlib.c ../src/bstrlib_helper.c -o $@

test_results: test_results.c ../src/results.c ../src/map.c ../src/ghash.c ../src/calculator.c ../src/stack.c
	$(CC) $(INCLUDES) -DCALCULATOR_AS_LIB test_results.c ../src/results.c ../src/map.c ../src/ghash.c ../src/calculator.c ../src/stack.c -o $@ -lm

test_config_getopt: test_config_getopt.c ../src/config_getopt.c ../src/bstrlib.c ../src/map.c ../src/ghash.c
	$(CC) $(INCLUDES) test_config_getopt.c ../src/config_getopt.c ../src/bstrlib.c ../src/map.c ../src/ghash.c -o $@

test_getopt_extra: test_getopt_extra.c ../src/getopt_extra.c ../src/bstrlib.c ../src/bstrlib_helper.c ../src/map.c ../src/ghash.c
	$(CC) $(INCLUDES) test_getopt_extra.c ../src/getopt_extra.c  ../src/bstrlib.c ../src/bstrlib_helper.c ../src/map.c ../src/ghash.c -o $@

clean:
	rm -f test_read_yaml_ptt test_results test_config_getopt test_getopt_extra
