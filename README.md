# ExtenDeck
<p align="center">
  <img src="https://github.com/Tetrapak0/ExtenDeck/blob/main/resources/icon.png?raw=true" alt="Icon"/>
</p>

## Index:
- [Acquisiton](#acquisition)
- [Build](#build)
  - [1. Clone repository](#1-clone-repository)
  - [2. Install dependencies](#2-install-dependencies)
  - [3. Configure CMake](#3-configure-cmake)
  - [4. Build](#4-build)
## Acquisition:
- [Prebuilt binaries](https://github.com/Tetrapak0/NexusShell/releases) ([Install dependencies first](#2-install-dependencies))
- [Build it yourself](#build)
## Build:
#### 1. Clone repository
##### 1.1 Install git:
- Arch-based: `sudo pacman -Syyu git`
- Debian-based: `sudo apt update && sudo apt install git`
- Fedora-based: `sudo dnf upgrade --refresh && sudo dnf install git`
- MSYS: `pacman -Syyu git`
##### 1.2 Clone
`git clone https://github.com/Tetrapak0/ExtenDeck --depth 1`
#### 2. Install dependencies
#### 2.1 Linux
>During development for Linux, ExtenDeck is only built using clang++.
- Arch-based: `sudo pacman -Syyu freetype2 cmake base-devel clang glfw dbus mesa libx11 libxrandr wayland`
- Debian-based: `sudo apt update && sudo apt install libfreetype-dev cmake build-essential clang libglfw3-dev libdbus-1-dev libgl1-dev libglu1-dev libx11-dev libxrandr-dev libwayland-dev`
- Fedora-based: `sudo dnf upgrade --refresh && sudo dnf install freetype-dev cmake clang glfw-devel dbus dbus-devel mesa-libGL-devel mesa-libGLU-devel libx11-devel libxrandr-devel wayland-devel && sudo dnf groupinstall "Development Tools" "Development Libraries"`
#### 2.2 Windows
##### Visual Studio / MSVC
> If you just want to build ExtenDeck, you can clone the repository and skip to [Configuring CMake](#3-configure-cmake)
> 
> For development with Visual Studio, follow these steps
1. Download [Premake](https://premake.github.io/)
2. Install [Git](https://git-scm.com/downloads)
3. [Clone repository](#1-clone-repository)
4. Open the cloned repository and run `premake5 vs2022` (make sure premake is in your PATH)
5. Open `ExtenDeck.sln` and build
##### MSYS / clang
1. Install MSYS2 from https://www.msys2.org/
2. Open the Clang64 environment and run the following commands to install dependencies:
    > Note that you may be asked to quit the session to update packages. In that case, please re-run the last command.
    ```shell
    pacman -Syyu mingw-w64-clang-x86_64-toolchain mingw-w64-clang-x86_64-glfw mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-freetype
    ```
3. Enter your MSYS home directory: `cd ~`
4. Install git via `pacman -S git`
5. [Clone repository](#1-clone-repository)
#### 3. Configure CMake
```shell
cd ExtenDeck
cmake -S . -B build -DCMAKE_BUILD_TYPE=MinSizeRel
```
#### 4. Build
```shell
cd build
cmake --build . --config MinSizeRel
```
