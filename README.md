# qbittorrent-portable
[![Build status](https://ci.appveyor.com/api/projects/status/1jbe4fogvjyg9vlk?svg=true)](https://ci.appveyor.com/project/myfreeer/qbittorrent-portable)
[![Download](https://img.shields.io/github/downloads/myfreeer/qbittorrent-portable/total.svg)](https://github.com/myfreeer/qbittorrent-portable/releases)
[![Latest Release](https://img.shields.io/github/release/myfreeer/qbittorrent-portable.svg)](https://github.com/myfreeer/qbittorrent-portable/releases/latest)
[![GitHub license](https://img.shields.io/github/license/myfreeer/qbittorrent-portable.svg)](LICENSE)

[dll-hijack](https://github.com/myfreeer/dll-hijack) based qbittorrent portable plugin.

## Usage
Put `iphlpapi.dll` **or** `version.dll` in the same folder of `qbittorrent.exe`.

## Compile
Requirements: git, cmake, mingw gcc, ninja.
Run `build.sh` or follow steps below:
```bash
git clone https://github.com/myfreeer/qbittorrent-portable.git --recursive
# or `git submodule update --init --recursive` after a regular clone
cd qbittorrent-portable
mkdir build
cd build
cmake -GNinja ..
ninja
```

## Credits
* https://github.com/shuax/GreenChrome
* https://github.com/TsudaKageyu/minhook
* https://github.com/myfreeer/dll-hijack