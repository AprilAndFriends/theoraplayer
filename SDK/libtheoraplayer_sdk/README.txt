This is the binary SDK for the Theora Playback Library Project

website: http://libtheoraplayer.sourceforge.net
Before using this SDK, it is recommended to check the project website for newer versions.

---------
COMPILING
---------
This SDK contains binaries of this library and the depending libraries (ogg, theora, vorbis, tremor).
The compiler version is indicated in the folder name. Be careful not to mix compilers! Although
it will most likely work, you'll have C++ runtime version problems etc. Even with the same major
version of Visual Studio, you may have different compiler versions (Service Pack upgrades etc.).

It is strongly recommended to compile your own version of this library. It's very simple and
straightforward given that this is an open source project. All the project files have been
set up and all dependencies included in the project's source repository. All you have to do
is compile the library with your compiler and use those binaries (or include the projects
in your own build system and compile it every time you compile your final product).

-----
USING
-----
Check the tutorial on the project website to learn how to use this library.

-------
LICENSE
-------
This project is released under the non-restrictive BSD License. Which means it's free to
use in pretty much any free or commercial scenario.

All that you have to keep in mind when using this library is to:
1) Include the name and website link of the project in your Credits page, readme file or
   any such document.
2) If possible, please let us know when and if you release a project that uses this library.
   contact email: kspes<{aT}>cateia<{.}>com
   There is a Showcase page on the project website where all known projects using this
   library are listed.
3) If you find a bug or better yet - fix it, or you have a feature request, please submit
   a ticket in the project Tracker (link available on the project website)
