echo Syncing msys2 packages...
C:\msys64\usr\bin\pacman -Sq --noconfirm --needed --noprogressbar --ask=20 mingw-w64-x86_64-ninja mingw-w64-i686-ninja

echo Building 64-bit version...
set MSYSTEM=MINGW64
call C:\msys64\usr\bin\bash -lc "cd \"$APPVEYOR_BUILD_FOLDER\" && exec ./build.sh"
mkdir mingw64
move /Y build_x86_64-w64-mingw32\version.dll  mingw64\version.dll
move /Y build_x86_64-w64-mingw32\iphlpapi.dll  mingw64\iphlpapi.dll
move /Y build_x86_64-w64-mingw32\winmm.dll  mingw64\winmm.dll

echo Building 32-bit version...
set MSYSTEM=MINGW32
call C:\msys64\usr\bin\bash -lc "cd \"$APPVEYOR_BUILD_FOLDER\" && exec ./build.sh"
mkdir mingw32
move /Y build_i686-w64-mingw32\version.dll  mingw32\version.dll
move /Y build_i686-w64-mingw32\iphlpapi.dll  mingw32\iphlpapi.dll
move /Y build_i686-w64-mingw32\winmm.dll  mingw32\winmm.dll

echo Packaging...
7z a -mx9 -r qbittorrent-portable.7z *.dll
move /Y mingw32\iphlpapi.dll .\32bit_iphlpapi.dll
move /Y mingw64\iphlpapi.dll .\64bit_iphlpapi.dll
echo Done.