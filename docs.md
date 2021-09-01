# CRender Documentaion

## What is CRender?
Crender is a render engine built by Jayjayli, Yohello, and Caio. It is written in C++ and well ray traces things. We built this because we were bored and wanted to make something even better.


## How to install?
### Windows
Download the latest release here, https://github.com/LegendWasTaken/CRender/release and start using it

### Linux

1) To build this project you will require [Intel Rendering Toolkit, specifically Embree 3.0 & Open Image Denoise](https://software.intel.com/content/www/us/en/develop/tools/oneapi/rendering-toolkit/download.html) (online version), [Intel Threaded Building Blocks](https://software.intel.com/content/www/us/en/develop/tools/oneapi/components/onetbb.html#gs.780vbz), CMake, and `libxi-dev build-essential xorg libglfw3 libglfw3-dev libxinerama-dev libxcursor-dev` (these are the packages for Debian/Ubuntu)

2) You will need to run `source /opt/intel/oneapi/setvars.sh` so it sets the global vars needed for building. If this doesn't work please look for this file, and source that file in the same terminal window you compile in.
3) Next you will need to run `cmake -DCMAKE_BUILD_TYPE=RELEASE .` this will build the cache
4) Finally you need to build it using `cmake --build . --target CRender`, now this will take a while.
5) Run `./CRender`

### Mac
Coming soon to ~~a theater near you~~ Mac

## How to open
### Windows
Open the exe
### Linux
Make sure you sourced the `setvars.sh` thingy in the intel libs. Then just run the executable


## FAQ
 Is it coming to Mac Soon?
 - Yes
 Can I use it on Linux?
 - Yes, just have to compile it
 Is it fast?
 - Very fast
