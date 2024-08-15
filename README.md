# likwid-bench

New version of likwid-bench

## Getting started

The new version of likwid-bench should contain some more features than the old likwid-bench:
- Multi-dimensional data structures
- Benchmark kernel related metrics
- Benchmark kernel definition in YAML files
- No external dependency (when compiled without LIKWID MarkerAPI support)

## Building

The build can be configured in the file `config.mk` like compiler to use, installation paths and features. By default, the GCC compiler is used. If you want to change settings for a compiler, check the appropriate `make/include_$(COMPILER).mk` file.

```
$ make
$ ./likwid-bench -h
###########################################
# likwid-bench - Micro-benchmarking suite #
###########################################
Option:
 -h/--help              : Print usage
 -V/--verbose           : Verbosity level (0 - 3)
 -t/--test <str>        : Name of testcase
 -f/--file <str>        : Filename for testcase
 -K/--kfolder <str>     : Benchmark folder
 -i/--iterations <str>  : Number of iterations for execution (either iterations/ runtime is allowed at a time)
 -r/--runtime <time>    : Runtime of benchmark (default in s, accepts ms, s, m, h, automatically determines iterations)

With the testcase name or the filename, the options might expand if the benchmark requires more input

 -w/--workgroup <str>  : A workgroup definition like S0:2 (multiple options allowed)

likwid-bench automatically detects the number of iterations (if not given) for the given or default
runtime.
```

## Running a benchmark kernel

If you want a list of all provided kernels, check the folder `kernels/$(ARCHITECTURE)`.

Kernels may define new parameters for the command line. To get the output for a kernel, specify it with `-t testname` or `-f yamlfile` and add `--help`.

```
$ ./likwid-bench -t <kernel> -h
###########################################
# likwid-bench - Micro-benchmarking suite #
###########################################

 -h/--help              : Print usage
 -V/--verbose           : Verbosity level (0 - 3)
 -t/--test <str>        : Name of testcase
 -f/--file <str>        : Filename for testcase
 -K/--kfolder <str>     : Benchmark folder
 -i/--iterations <str>  : Number of iterations for execution (either iterations/ runtime is allowed at a time)
 -r/--runtime <time>    : Runtime of benchmark (default in s, accepts ms, s, m, h, automatically determines iterations)

likwid-bench automatically detects the number of iterations (if not given) for the given or default
runtime.

 -w/--workgroup <str>   : A workgroup definition like S0:2 (multiple options allowed)
 -N <value>             : Size of array that should be loaded (Options: kb, MB, GB, TB required)
```



## Development plan

- [x] Parsing of workgroup(s) `S0:2`
- [x] Compile assembly from YAML file to object code
- [x] Allocate streams with given sizes
- [x] Create threads according to workgroup(s)
- [ ] Run benchmark (`dlopen`, etc.)
- [ ] Evaluate given metrics using given variables and runtime measurements
- [ ] Print out results in various formats

## Decisions
- YAML instead of JSON. The parser is much simpler and integrated. With JSON, there would be a dependency to a JSON parser lib like [JSMN](https://github.com/zserge/jsmn). It's valid YAML but quite similar to the PTT format used before.

