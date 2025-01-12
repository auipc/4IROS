# 4IR

A crummy kernel presented as an OS. Targets x86-64.

There was an ARM port, which was lost. 

(I'm quite fond of ARM's design, not the architecture. A nice detail of the implementation is that large pages are supported simply by varying the depth at which you put a page.)

## Building

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.cmake ..
```
