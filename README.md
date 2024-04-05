# Fusion Game Engine
A Game engine made with Vulkan in C.
## Plan
Fusion is a 3D game engine, built with the least number of dependencies possible. Since Fusion is still in it's VERY early stage of living. The current goal is to just make a simple platformer game with it. That means Audio for music, simple animations, physics (I dread this the most), and collision triggers. This goal obviously will change once this one is reached.
## Platform support
Windows and Linux are officially supported. Specifically Windows 11 and Arch Linux since those are the two platforms that Fusion is developed on.

I currently don't have any Mac computers, but because of the way the project is structured it will be easy to add on later on.

## Prerequisites
Each platform has some prerequisites to develop on.

### Prerequisites for All Platforms
Make sure you have Clang and the Vulkan SDK installed: (You can use a different compiler, but it's up to you to make the compile files)

-   Clang:  [https://releases.llvm.org/download.html](https://releases.llvm.org/download.html)
-   Vulkan SDK:  [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/)
### Prerequisites for Windows
-   Make for Windows:  [https://gnuwin32.sourceforge.net/packages/make.htm](https://gnuwin32.sourceforge.net/packages/make.htm)  (Yes I mean the one that updated in 2006, it still works)
-   Visual Studio Community (acts as backend for clang), 2019 or above [https://visualstudio.microsoft.com/vs/community/](https://visualstudio.microsoft.com/vs/community/)

### Prerequisites for Linux
Install these via your distro's package manager

-   `make`
-   `libx11-dev`
-   `libxkbcommon-x11-dev`
-   `libx11-xcb-dev`
