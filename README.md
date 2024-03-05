# BenchmarkScripts-TeaScript-VS-ChaiScript-VS-Jinx
This repository is intended for host various benchmarking code for 
benchmark script languages available in and for C++ in different disciplines.

The script languages under investigation are primary 
- [TeaScript](https://tea-age.solutions/teascript/overview-and-highlights/)
- [ChaiScript](https://chaiscript.com/)
- [Jinx](https://jamesboer.github.io/Jinx/)

(these are all modern C++ and available as header only)

and secondary
- [Lua](https://www.lua.org/)

Other languages, like [Python](https://www.python.org/), and pure C++ as a reference mark, might be used as well.

# The Benchmarks

## Fibonacci Benchmark

In this benchmark a fibonacci number (default 25) must be caculated, either recursively or iteratively.

## Variable Lookup Benchmark

This benchmark attempts to test vaurious operations for variable storage like lookup, delete, etc.
Actually this is a TeaScript only benchmark for comparing different TeaScript versions.

## BufferOverhead Benchmark

In this benchmark a 32 bit RGBA image buffer with either Full HD or UHD resolution must be filled pixel by pixel.

# Usage
- You need all script languages, which you want to test, as source (header only).
  - you can disable script languages with configuration macros at the top of the benchmark code.
- You must change the include pathes in the Visual Studio 2022 project files.
- configure the benchmark as you wish with the macros at top of the source code.
- compile and run the benchmark in Release Build.

# BufferOverhead Benchmark Result
A result of the BufferOverhead Benchmark between ChaiScript and TeaScript can be found in the release article of TeaScript 0.13.0:<br>
[TeaScript 0.13.0](https://tea-age.solutions/2024/03/04/release-of-teascript-0-13-0/)

# Fibonacci Benchmark Result
A result of the recursive Fibonacci Benchmark can be found here:
[TeaScript VS ChaiScript Fibonacci Benchmark](https://tea-age.solutions/2023/01/08/teascript-vs-chaiscript-fibonacci-benchmark/)

*Note: That benchmark was done before Jinx was included as a testable script language.*

# A note to TeaScript
TeaScript is now public available as a C++ header only Library. <br>
You can download the latest release here: https://tea-age.solutions/downloads/ <br>
or use the snapshot on Github: https://github.com/Florian-Thake/TeaScript-Cpp-Library

# Script Comparison
A comparison between C++ scripting libraries is available here:<br>
https://tea-age.solutions/2023/01/31/script-language-comparison-for-embed-in-cpp/

# License 
SPDX-FileCopyrightText: Copyright (C) 2024 Florian Thake, <contact |at| tea-age.solutions>.

SPDX-License-Identifier: MIT
