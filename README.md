
<h1 align="center">Heap memory allocator</h1>
<p align="center">
  <img src="https://img.shields.io/badge/Language-C-A8B9CC?style=flat-square&logo=c&logoColor=white"/>
  <img src="https://img.shields.io/badge/Platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black"/>
</p>

## Overview

A single-threaded memory allocator implementing `malloc` and `free` in C. The implementation is base on dlmalloc.
The memory is managed by `sbrk()` using chunk-based heap with size-aggregated bins and coalescing.

## Usage
```c
#include "allocator.h"

int main() {
  void*ptr = allocate(256);
  if(!ptr) return 1;

  free_memory(ptr)
  return 0;
}
```
Compile:
```bash
gcc -o program main.c allocator.c
```

## License
This project is licensed under the MIT License.
