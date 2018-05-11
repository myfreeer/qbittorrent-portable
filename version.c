#include "version.h"
#include "minhook/include/MinHook.h"
#include "dir.h"
void DLLHijackAttach(bool isSucceed) {
    if (isSucceed) {
        // MessageBox(NULL, TEXT("DLL Hijack Attach Succeed!"), TEXT(DLL_NAME " DLL Hijack Attach"), MB_OK);
        MH_Initialize();
        CreateHook();
    }
}

void DLLHijackDetach(bool isSucceed) {
    if (isSucceed)
        MH_Uninitialize();
}
