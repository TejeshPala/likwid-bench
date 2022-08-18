# The architecture of likwid-bench


## Command line parsing

The execution of likwid-bench is controlled by command line options to the extent that the command line options vary based on other options. The command line is simply parsed twice. First some basic options are collected including the kernel name or file. Based on the information in the kernel file (the name resolves to `kernels/<arch>/<name>.yaml`), the command line is parsed again to get kernel specific options. This was done because some kernels don't need any input while others need a lot of configuration.

Example: The `load` kernel requires a single data stream which length is unknown. So, the `load` kernel adds an option `-N <size_in_bytes>` to the command line options to get the size for its operation.

## Kernel files

The kernel files are written in YAML and consist of two parts:
- The kernel description in YAML
- The kernel code in assembly

### Kernel description

```yaml
---
- Name: <name of your kernel>
- Description: <description of your kernel>
- Parameters:
  - <parameter name>: <this name can used in the assembly below>
      description: <parameter description>
      options: <list of options used while parsing, see below for parameter options>
        - bytes <parameter is in bytes (1000 == 1000B) and scales can be used 'kB', 'MB', ...>
        - required <this parameter is required>
- Streams:
  - <stream name>: <this name can used in the assembly below>
      dimensions: <number of dimensions>
      datatype: <data type of stream like 'double'>
      dimsizes:
        - 100 <you can use fixed sizes or a variable name>
        - <parameter name> <read from command line>
  - STR0:
      dimensions: 1
      datatype: double
      dimsizes:
        - N
- Variables: <list of key/value pairs with fixed values>
  <key 1>: <value 1>
  <key 2>: <value 2>

- Metrics: <list of name/formula pairs used for output>
  <metric name 1>: <formula1>
  <metric name 2>: <key1>*ITER/TIME <the defined variables can be used here, as well as ITER and TIME>

- Threads: <define how the kernel should be run in multi-threaded environments>
  offsets: <one offset per dimension>
    - THREAD_ID*(N/NUM_THREADS) <specify the starting offset per thread, variables can be used as well as THREAD_ID and NUM_THREADS>
  sizes: <one size per dimension>
    - N/NUM_THREADS <specify the stream size for each thread>
...
```

### Assembly code

The assembly code is almost valid assembly in Intel syntax. There are some strings in the code that need to be replaced before it is really valid assembly. Almost all settings from the kernel description part are accessible here and will be replaced before compilation.

```
LOOP(loop, rax=0, <, rdi=N, UNROLL_FACTOR)
movsd    xmm0, [STR0 + rax * 8]
movsd    xmm1, [STR0 + rax * 8 + 8]
movsd    xmm2, [STR0 + rax * 8 + 16]
movsd    xmm3, [STR0 + rax * 8 + 24]
movsd    xmm4, [STR0 + rax * 8 + 32]
movsd    xmm5, [STR0 + rax * 8 + 40]
movsd    xmm6, [STR0 + rax * 8 + 48]
movsd    xmm7, [STR0 + rax * 8 + 56]
LOOPEND(loop)
```

- `LOOP(loop, ...)` is an opening keyword defined by `likwid-bench` and will be replaced with the architecture specific loop logic
- `N` is a parameter read from command line
- `UNROLL_FACTOR` is a variable from the kernel description
- `STR0` is resolved to a register which contains the start index of a specified stream (see above for an example)
- `LOOPEND(loop)` is the closing keyword defined by `likwid-bench` and will be replaced with the architecture specific loop logic

## Translating kernel files for execution

The kernel files contain a block of information and a list of assembly operations including some `likwid-bench` specific keywords like `LOOP`.

### Resolving keywords

A keyword in LIKWID bench consists always of an opening and a closing keyword and both have a name parameter as first argument. This way it is easy to find opening and closing keywords in the code.

```
KEYWORD(label, ...)
<code>
KEYWORDEND(label, ...)
```

The remaining arguments are keyword specific.

In the first step, the assembly code parsed. When a opening keyword (with label) and a corresponding closing keyword (with same label) is found, the code is split into three parts: prior to the opening keyword, the keywords with lines in between and the code after the closing keyword. The keyword part is then transformed. Afterwards the transformed code is recursively submitted to the parser again. If there is no keyword found in this part, the code is simply returned without modification. If there are still keywords in the code, search again and split into three parts again. Due to the recursion, nested keywords are possible. Moreover, multiple keyword-guarded areas can be put in one code.

#### `LOOP() ... LOOPEND()` keyword


### Replacing variables

After the resolution of the keywords, the code is almost ready to be compiled. The variables usable in the assembly need to be replaced with actual values. While some are static and can be hardcoded into the kernel, some are runtime specific like the stream addresses.

First all variables are collected from various data structures and put in two lists for the keys and values. Afterwards the maximal length of the keys is determined. This is done to replace the longer keys first before trying to replace the shorter keys. A shorter key might be a substring of a longer key.

### Compiling code

After all the translations and replacements, the code is ready to be compiled. `likwid-bench` searches for known compilers like `gcc`, `icc`, `icx`, ... and uses the first found. The compiler cannot do any optimizations on the code anymore, it's only assembly, so as long as the compiler can create object code from assembly, it can be used.

