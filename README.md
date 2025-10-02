# 4IR

![But that is the beginning of a new story—the story of the gradual renewal of a man, the story of his gradual regeneration, of his passing from one world into another, of his initiation into a new unknown life.](/media/screen.png)

A crummy kernel presented as an OS. Targets x86-64.

## Building

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-clang-x64.cmake ..
```

## TODO

Port SVGALib so display isn't QEMU/Bochs only.
We have to modify EFlags slightly to let this happen since SVGALib relies on the IO bus, also need a /dev/mem interface.
