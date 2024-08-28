# clock

## How to Build

Download the repo and navigate to the root folder.
```
sudo apt update
sudo apt install -y libsdl2-dev libsdl2-ttf-dev inkscape cmake build-essential
cmake -B build -DCMAKE_BUILD_TYPE=Release -S .
cmake --build build --config Release
(cd build && ctest --build-config Release)
(cd build && cpack --build-config Release)

```
Find the deb installer in `build` folder.
