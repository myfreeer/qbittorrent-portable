#include <tchar.h>
#include <shlobj.h>
#include <stdbool.h>
#include "minhook/include/MinHook.h"

typedef HRESULT (WINAPI *pSHGetKnownFolderPath)(
        _In_     REFKNOWNFOLDERID rfid,
        _In_     DWORD            dwFlags,
        _In_opt_ HANDLE           hToken,
        _Out_    PWSTR            *ppszPath
);

pSHGetKnownFolderPath OriginalSHGetKnownFolderPath = NULL;

HRESULT WINAPI HookSHGetKnownFolderPath(
        _In_     REFKNOWNFOLDERID rfid,
        _In_     DWORD            dwFlags,
        _In_opt_ HANDLE           hToken,
        _Out_    PWSTR            *ppszPath
) {
    if (IsEqualGUID(rfid, &FOLDERID_Programs) ||
            IsEqualGUID(rfid, &FOLDERID_LocalAppData) ||
            IsEqualGUID(rfid, &FOLDERID_RoamingAppData)) {
        TCHAR szDir[MAX_PATH] = { 0 };
        TCHAR* szEnd = NULL;
        GetModuleFileName(NULL, szDir, MAX_PATH);
        szEnd = _tcsrchr(szDir, _T('\\'));
        *(szEnd) = 0;
        size_t length = szEnd - szDir;
        PWSTR dirPath = CoTaskMemAlloc((length+1)*sizeof(TCHAR));
        if (dirPath == NULL) {
            return E_FAIL;
        }
        _tcscpy(dirPath, szDir);
        *ppszPath = dirPath;
#ifdef HookDebug
        MessageBox(NULL, dirPath, (LPCTSTR) _T("SHGetKnownFolderPath Hook Path"), MB_OK);
#endif
        return S_OK;
    } else return OriginalSHGetKnownFolderPath(rfid, dwFlags, hToken, ppszPath);
}

void DLLHijackAttach(bool isSucceed) {
    if (isSucceed) {
#ifdef HookDebug
        MessageBox(NULL, TEXT("DLL Hijack Attach Succeed!"), TEXT(DLL_NAME " DLL Hijack Attach"), MB_OK);
#endif
        MH_Initialize();
        HMODULE shell32 = LoadLibrary((LPCWSTR) _T("shell32.dll"));
        void *SHGetKnownFolderPath = GetProcAddress(shell32, "SHGetKnownFolderPath");
        MH_STATUS status = MH_CreateHook(SHGetKnownFolderPath, &HookSHGetKnownFolderPath,
                                         (LPVOID *) &OriginalSHGetKnownFolderPath);
        if (status == MH_OK) {
            status = MH_EnableHook(SHGetKnownFolderPath);
#ifdef HookDebug
            if (status == MH_OK) {
                MessageBox(NULL,  (LPCTSTR) _T("SHGetKnownFolderPath Hook Succeed"), (LPCTSTR) _T("SHGetKnownFolderPath Hook"), MB_OK);
            } else {
                MessageBox(NULL,  (LPCTSTR) _T("SHGetKnownFolderPath Hook Enable Fail"), (LPCTSTR) _T("SHGetKnownFolderPath Hook"), MB_OK);
            }
        } else {
            MessageBox(NULL,  (LPCTSTR) _T("SHGetKnownFolderPath Hook Fail"), (LPCTSTR) _T("SHGetKnownFolderPath Hook"), MB_OK);
#endif
        }

    }
}

void DLLHijackDetach(bool isSucceed) {
    if (isSucceed) {
        MH_Uninitialize();
#ifdef HookDebug
        MessageBox(NULL, TEXT("DLL Hijack Detach Succeed!"), TEXT(DLL_NAME " DLL Hijack Detach"), MB_OK);
#endif
    }
}
