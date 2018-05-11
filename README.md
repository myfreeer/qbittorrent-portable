# qbittorrent-portable
[dll-hijack](https://github.com/myfreeer/dll-hijack) based qbittorrent portable plugin.

## Usage
Put `version.dll` in the same folder of `qbittorrent.exe`.

## Compile
Requirements: git, cmake, mingw gcc, ninja.
Run `build.sh` or follow steps below:
```bash
git clone https://github.com/myfreeer/qbittorrent-portable.git --recursive
# or `git submodule update --init  --recursive` after a regular clone
cd qbittorrent-portable
mkdir build
cd build
cmake -GNinja ..
ninja
```

## Credits
* https://github.com/shuax/GreenChrome
* https://github.com/myfreeer/dll-hijack