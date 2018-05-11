echo Syncing msys2 packages...
C:\msys64\usr\bin\pacman -Sq --noconfirm --needed --noprogressbar --ask=20 mingw-w64-x86_64-ninja mingw-w64-i686-ninja

echo Building 64-bit version...
set MSYSTEM=MINGW64
call C:\msys64\usr\bin\bash -lc "cd \"$APPVEYOR_BUILD_FOLDER\" && exec ./build.sh"
mkdir mingw64
move /Y build_x86_64-w64-mingw32\version.dll  mingw64\version.dll

echo Building 32-bit version...
set MSYSTEM=MINGW32
call C:\msys64\usr\bin\bash -lc "cd \"$APPVEYOR_BUILD_FOLDER\" && exec ./build.sh"
mkdir mingw32
move /Y build_i686-w64-mingw32\version.dll  mingw32\version.dll

echo Packaging...
7z a -mx9 -r binary.7z *.dll
echo Done.