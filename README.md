Flux Engine
===========
Flux engine is a WORK IN PROGRESS game engine intended for small 3D games

Flux Engine is currently not in a usable state for, well, _anything_

Goals
-----
Here are the main goals of this project

1. Run well on low end hardware
2. Run well on the web through WebGL 2
3. Be as small as possible

Flux engine is not intended to compete with Unreal, or even Unity. It is entended for making small 3d games, with "bad" graphics, which can run anywhere, on anything.

As long as "anything" has OpenGL ES 3 support

Building
========
Everything uses submodules, so building is as simple as running CMake:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```