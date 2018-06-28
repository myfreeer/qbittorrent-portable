#include <tchar.h>
#include <shlobj.h>
#include <stdbool.h>
#include "minhook/include/MinHook.h"

#define CreateMinHook(func) MH_CreateHook(func, &Hook##func, (LPVOID *) &Original##func)
#ifdef HookDebug
#define EnableMinHook(func, status) if ((status) == MH_OK) { \
    (status) = MH_EnableHook(func); \
        if ((status) == MH_OK) { \
            MessageBox(NULL,  (LPCTSTR) _T(#func " Hook Succeed"), (LPCTSTR) _T(#func " Hook"), MB_OK); \
        } else {\
            MessageBox(NULL,  (LPCTSTR) _T(#func " Hook Enable Fail"), (LPCTSTR) _T(#func " Hook"), MB_OK); \
        }\
    } else { \
        MessageBox(NULL,  (LPCTSTR) _T(#func " Hook Fail"), (LPCTSTR) _T#func (" Hook"), MB_OK); \
    }
#else // HookDebug
#define EnableMinHook(func, status) if ((status) == MH_OK) { \
    (status) = MH_EnableHook(func); \
    }
#endif // HookDebug

/**
 * GetModulePath
 * @param {TCHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path
 */
DWORD WINAPI GetModulePath(TCHAR *pDirBuf, DWORD bufSize) {
	TCHAR* szEnd = NULL;
	GetModuleFileName(NULL, pDirBuf, bufSize);
	szEnd = _tcsrchr(pDirBuf, _T('\\'));
	*(szEnd) = 0;
	return szEnd - pDirBuf;
}

/**
 * GetModulePathW
 * @param {WCHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path
 */
DWORD WINAPI GetModulePathW(WCHAR *pDirBuf, DWORD bufSize) {
    WCHAR* szEnd = NULL;
    GetModuleFileNameW(NULL, pDirBuf, bufSize);
    szEnd = wcsrchr(pDirBuf, L'\\');
    *(szEnd) = 0;
    return szEnd - pDirBuf;
}

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
            IsEqualGUID(rfid, &FOLDERID_Desktop) ||
            IsEqualGUID(rfid, &FOLDERID_Documents) ||
            // IsEqualGUID(rfid, &FOLDERID_Downloads) ||
            IsEqualGUID(rfid, &FOLDERID_LocalAppDataLow) ||
            IsEqualGUID(rfid, &FOLDERID_Profile) ||
            IsEqualGUID(rfid, &FOLDERID_RoamingAppData)) {
        WCHAR szDir[MAX_PATH] = { 0 };
        size_t length = GetModulePathW(szDir, MAX_PATH);
        PWSTR dirPath = CoTaskMemAlloc((length+1)*sizeof(TCHAR));
        if (dirPath == NULL) {
            return E_FAIL;
        }
        wcscpy(dirPath, szDir);
        *ppszPath = dirPath;
#ifdef HookDebug
        MessageBox(NULL, dirPath, (LPCTSTR) _T("SHGetKnownFolderPath Hook Path"), MB_OK);
#endif
        return S_OK;
    } else return OriginalSHGetKnownFolderPath(rfid, dwFlags, hToken, ppszPath);
}

typedef WINBOOL (WINAPI *pSHGetSpecialFolderPathW) (
        HWND   hwndOwner,
        _Out_ LPWSTR lpszPath,
        _In_  int    csidl,
        _In_  BOOL   fCreate
);

pSHGetSpecialFolderPathW OriginalSHGetSpecialFolderPathW = NULL;

WINBOOL WINAPI HookSHGetSpecialFolderPathW(
        HWND   hwndOwner,
        _Out_ LPWSTR lpszPath,
        _In_  int    csidl,
        _In_  BOOL   fCreate
) {
    if (csidl == CSIDL_APPDATA ||
        csidl == CSIDL_PROGRAMS ||
        csidl == CSIDL_COMMON_APPDATA ||
        csidl == CSIDL_PERSONAL ||
        csidl == CSIDL_DESKTOP ||
        csidl == CSIDL_MYDOCUMENTS ||
        csidl == CSIDL_PROFILE ||
        csidl == CSIDL_COMMON_DOCUMENTS ||
        csidl == CSIDL_LOCAL_APPDATA) {
        if (lpszPath == NULL) {
            return FALSE;
        }
        GetModulePathW(lpszPath, MAX_PATH);
#ifdef HookDebug
        MessageBox(NULL, lpszPath, (LPCTSTR) _T("SHGetSpecialFolderPath Hook Path"), MB_OK);
#endif
        return TRUE;
    } else return OriginalSHGetSpecialFolderPathW(hwndOwner, lpszPath, csidl, fCreate);
}


void DLLHijackAttach(bool isSucceed) {
    if (isSucceed) {
        MH_Initialize();
#ifdef HookDebug
        MessageBox(NULL, TEXT("DLL Hijack Attach Succeed!"), TEXT(DLL_NAME " DLL Hijack Attach"), MB_OK);
#endif
        HMODULE shell32 = LoadLibrary((LPCWSTR) _T("shell32.dll"));
        void *SHGetKnownFolderPath = GetProcAddress(shell32, "SHGetKnownFolderPath");
        MH_STATUS status = CreateMinHook(SHGetKnownFolderPath);
        EnableMinHook(SHGetKnownFolderPath, status);
        void *SHGetSpecialFolderPathW = GetProcAddress(shell32, "SHGetSpecialFolderPathW");
        status = CreateMinHook(SHGetSpecialFolderPathW);
        EnableMinHook(SHGetSpecialFolderPathW, status);
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
