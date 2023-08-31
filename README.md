#### 4IR

This is the 4th revision of my operating system.

##### Goals
- Monolithic kernels are boring, non-unique, prone to errors, and are an effective cliche for Unix-likes. We should try to get as much distance from that as possible.
- x86-64 at some point


##### Building

```bash
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.cmake ..
```


#### Roadmap

##### Filesystem

If ext2 support is implemented, it probably won't be phased out for a while. Instead, we should use the time spent implementing ext2 as time to design and implement a filesystem. The filesystem should be unique in terms of features and not something that something that gets added to the evergrowing pile of useless filesystems that operating systems will forever add or phase out support for.

The filesystem will start as a FUSE layer, most likely written in portable C++ allowing us to mount it on systems other than our own.

Some of the features that really should be added are listed here

[ ] - Copy on Write
[ ] - Have an algorithm that finds the best-fit for storing the file (if it has a lot of holes, use some type of range header system similar to RLE but not exactly like it)
[ ] - Tree based (INodes should be able to be shifted around, without issue)
[ ] - Journaling, although this can be delayed until the step of it not being a toy OS is reached. 
[ ] - Permissions
[ ] - Symlinks (CoW support probably implies we have the framework forthis implemented)

##### Better Exception handling

##### Non-sucky malloc

##### Better and portable LibC

Our kernel already has hints of LibC, we should move that code over to a hybrid LibC/LibKern.

##### Ref count objects in the kernel
