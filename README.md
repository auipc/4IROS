#### 4IR

This is the 4th revision of my operating system.

##### Goals
- Monolithic kernels are boring, non-unique, prone to errors, and are an effective cliche for Unix-likes. We should try to get as much distance from that as possible.
- x86-64 at some point


##### Building

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.cmake ..
```


##### TODO

- [ ] Some type of way to handle exceptions in the kernel, maybe a panic function?
- [ ] Better memory allocator, that won't intersect with existing memory too.
- [x] MMU
- [x] Interrupts
