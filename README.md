# vulkan_engine

## build step:

### Windows
- download vulkan sdk
- set up environment path to VULKAN_SDK
- run cmake --preset DebugAsanWindows

  - build with cmake:
    * run cmake --build build

  - build with visual studio:
    * need visual studio 2022
    * if not visual studio 2022 -> need to change directories to correct glfw lib directories
    * in build, open demo.sln
    * set demo as "Start up project"
    * click build/run

  - build with Clion:
    * Open vulkan-engine/CMakeLists.txt
    * "Edit CMake Profiles"
    * click on the profile you want and "Enable profile"
    * change profile to the profile you want and build

### MacOs
- TBD


