This is where dependency lib's are stored.
Compile and put libogg,libvorbis and libtheora lib files here for easiest compilation.
All msvc projects have been set to look for libraries in this folder.

Check on sourceforge, there will likely be a compiled dependency package available if
you don't feel like compiling them yourself. These were not included into the source code
repository because of:
1) Don't want to bloat the source code repository (arround 5 MB extra!)
2) Some users might want to compile their own dependencies
3) Not necesarry on other compilers (see 1))
