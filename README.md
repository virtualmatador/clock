# clock

## How to Build

Download the repo and navigate to the root folder.

### Linux

```
sudo apt update
sudo apt install -y libsdl3-dev libsdl3-ttf-dev inkscape imagemagick cmake build-essential
cmake -B build -DCMAKE_BUILD_TYPE=Release -S .
cmake --build build --config Release
(cd build && ctest --build-config Release)
(cd build && cpack --build-config Release)

```
Find the `deb` installer in `build` folder.

### Windows

Install `inkscape`, `imagemagick`, `cmake`, and `C++` build tools.
Download SDL3 and SDL3_ttf libraries and add the path to `CMAKE_PREFIX_PATH`.
```
cmake -B build -DCMAKE_BUILD_TYPE=Release -S .
cmake --build build --config Release
(cd build && ctest --build-config Release)
(cd build && cpack --build-config Release)

```
Find the `msi` installer in `build` folder.
