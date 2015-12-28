BDE Allocator Benchmarking Tools
================================
This is a group of programs attempting to reproduce the results found in
[N4468](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4468.pdf),
"On Quantifying Allocation Strategies".

These programs depend upon:
  * Clang version 3.6 or later, or gcc version 5.1 or later, built with
   support for link-time-optimization (LTO, see FAQ)
  * For clang, libc++ version 3.6, or later
  * For gcc 5.1, libstdc++ patched according to:
   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66055
  * [BDE Tools](https://github.com/bloomberg/bde-tools), which contains a
    custom build system based on [waf](https://github.com/waf-project/waf)
  * If building for LTO, /usr/bin/ld indicating ld.gold

A suitably-configured system can be built quickly using
[Docker](https://docker.com); see the 'Docker' section below for instructions.

To configure,
```
  $ vi Makefile    # configure per instructions
```

To build,
```
  $ make benchmark_##
```

To run benchmarks,
```
  $ ./benchmark_##
```


Docker
======
Two Dockerfiles have been provided which can be used to construct images with
all necessary tools and configuration for building these programs. One file 
builds a Debian 'testing' (Stretch) based image, and the other builds an Ubuntu
15.04 (Vivid) based image. All commands should be run from within the 
benchmarks/allocators dirrectory of the repo.

Note: There are some known bugs for Docker on Windows that will prevent the
below commands from working out of the box. See the final section for further
explanation

Debian Stretch
--------------
To use the Debian Stretch Dockerfile, follow these steps:
```
# Build an image called 'debian-stretch-clang'
docker build -f ./docker/debian-stretch-clang --rm -t debian-stretch-clang ./docker

# Build the BDE library and benchmarks using the image
docker run --rm -v $(git rev-parse --show-toplevel):/src -w /src/benchmarks/allocators debian-stretch-clang make benchmark_##

# Run the benchmarks
docker run --rm -v $(git rev-parse --show-toplevel):/src -w /src/benchmarks/allocators debian-stretch-clang ./benchmark_##
```

Ubuntu Vivid
------------
To use the Ubuntu Vivid Dockerfile, follow these steps:
```
# Build an image called 'ubuntu-vivid-clang'
docker build -f ./docker/ubuntu-vivid-clang --rm -t ubuntu-vivid-clang ./docker

# Build the BDE library and benchmarks using the image
docker run --rm -v $(git rev-parse --show-toplevel):/src -w /src/benchmarks/allocators ubuntu-vivid-clang make benchmark_##

# Run the benchmarks
docker run --rm -v $(git rev-parse --show-toplevel):/src -w /src/benchmarks/allocators ubuntu-vivid-clang ./benchmark_##
```

Windows Example
---------------
Run the following command:
```
git rev-parse --show-toplevel
```
The output should look something like this:
```
C:/Users/gblea/Allocator-Benchmarks
```
You will need to change the slashes and drive letter to match the syntax
expected by the docker terminal:
```
/c/Users/gblea/Allocator-Benchmarks
```
Replace ```$(git rev-parse --show-toplevel)``` with the modified path in the
proveded docker commands, to get something like this:
```
docker run --rm -v /c/Users/gblea/Allocator-Benchmarks:/src -w /src/benchmarks/allocators debian-stretch-clang make benchmark_1
```
Finally, add a second forward slash to all root-level forward slashes in
the command:
```
docker run --rm -v //c/Users/gblea/Allocator-Benchmarks://src -w //src/benchmarks/allocators debian-stretch-clang make benchmark_1
```

FAQ
===
Q1: What about tcmalloc?

A1: In all our tests, tcmalloc was substantially slower than the default
    new/delete on the target platform.

Q2: Why do these depend on recent clang++/libc++ or g++/libstdc++?

A2: The tests, particularly growth.cc and locality.cc, depend on features of
  C++14: specifically, the containers' comprehensive observance of allocator-
  traits requirements to direct their memory management.

Q3: How can I build these with a non-LTO-enabled toolchain?

A3: Comment out the ```LTO``` variable value in ```Makefile```.
    
