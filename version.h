#define DLL_NAME "version"
#define VER_H
#include "hijack.h"

EXPORT GetFileVersionInfoA() NOP_FUNC(1);
EXPORT GetFileVersionInfoByHandle() NOP_FUNC(2);
EXPORT GetFileVersionInfoExA() NOP_FUNC(3);
EXPORT GetFileVersionInfoExW() NOP_FUNC(4);
EXPORT GetFileVersionInfoSizeA() NOP_FUNC(5);
EXPORT GetFileVersionInfoSizeExA() NOP_FUNC(6);
EXPORT GetFileVersionInfoSizeExW() NOP_FUNC(7);
EXPORT GetFileVersionInfoSizeW() NOP_FUNC(8);
EXPORT GetFileVersionInfoW() NOP_FUNC(9);
EXPORT VerFindFileA() NOP_FUNC(10);
EXPORT VerFindFileW() NOP_FUNC(11);
EXPORT VerInstallFileA() NOP_FUNC(12);
EXPORT VerInstallFileW() NOP_FUNC(13);
EXPORT VerLanguageNameA() NOP_FUNC(14);
EXPORT VerLanguageNameW() NOP_FUNC(15);
EXPORT VerQueryValueA() NOP_FUNC(16);
EXPORT VerQueryValueW() NOP_FUNC(17);
