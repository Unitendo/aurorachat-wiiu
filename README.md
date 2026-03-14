<h1 align="center">Welcome to the aurorachat repository!</h1>
This is the Wii U client for Aurorachat.<br>
For more clients and stuff, see the <a href="https://github.com/Unitendo/aurorachat">main repo</a>.
The license, code of conduct, and security/contributing guidelines in the main repo also apply here.

<br>This repository is <b>open</b> for contributions! If you'd like to, you may open a PR or an issue, contributing helps us as we develop aurorachat!

<h1 align="center">How to build aurorachat</h1>

1. Make sure you have devkitpro set up (https://devkitpro.org/wiki/Getting_Started)

2. Install the required dependencies.

```
sudo pacman -S wut wut-tools ppc-pkg-config wiiu-pkg-config devkitPPC wiiu-sdl2 wiiu-sdl2_ttf wiiu-sdl2_image ppc-freetype ppc-harfbuzz ppc-libpng ppc-zlib bzip2 ppc-brotli
```
or
```
sudo dkp-pacman -S wut wut-tools ppc-pkg-config wiiu-pkg-config devkitPPC wiiu-sdl2 wiiu-sdl2_ttf wiiu-sdl2_image ppc-freetype ppc-harfbuzz ppc-libpng ppc-zlib bzip2 ppc-brotli
```

3. You will also need to install and download libromfs-wiiu from the GitHub and even though you installed SDL2 from pacman, it may be out of date.
```
git clone https://github.com/yawut/libromfs-wiiu.git
cd libromfs-wiiu
make
sudo -E make install
cd ..

git clone https://github.com/devkitPro/SDL.git
cd SDL
git checkout wiiu-sdl2-2.28
/opt/devkitpro/portlibs/wiiu/bin/powerpc-eabi-cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$DEVKITPRO/portlibs/wiiu
cmake --build build
sudo cmake --install build
cd ..
```

If you got no errors you can safely remove the folders
```
rm -rf libromfs-wiiu
rm -rf SDL
```

4. Once the environment is setup:
```
git clone https://github.com/ItsFuntum/aurorachat-wiiu.git
cd aurorachat-wiiu
mkdir build && cd build
/opt/devkitpro/portlibs/wiiu/bin/powerpc-eabi-cmake ../ -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make
```
