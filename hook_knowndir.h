#include <tchar.h>
#include <shlobj.h>
#include <stdbool.h>
#include <string.h>
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

#define isHookCsidl(csidl)  ((csidl) == CSIDL_APPDATA || \
        (csidl) == CSIDL_PROGRAMS || \
        (csidl) == CSIDL_COMMON_APPDATA || \
        (csidl) == CSIDL_PERSONAL || \
        (csidl) == CSIDL_DESKTOP || \
        (csidl) == CSIDL_MYDOCUMENTS || \
        (csidl) == CSIDL_PROFILE || \
        (csidl) == CSIDL_COMMON_DOCUMENTS || \
        (csidl) == CSIDL_LOCAL_APPDATA)

#define isHookRfid(rfid) (IsEqualGUID(rfid, &FOLDERID_Programs) || \
            IsEqualGUID(rfid, &FOLDERID_LocalAppData) || \
            IsEqualGUID(rfid, &FOLDERID_Desktop) || \
            IsEqualGUID(rfid, &FOLDERID_Documents) || \
            IsEqualGUID(rfid, &FOLDERID_LocalAppDataLow) || \
            IsEqualGUID(rfid, &FOLDERID_Profile) || \
            IsEqualGUID(rfid, &FOLDERID_RoamingAppData))
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
 * GetModulePathA
 * @param {CHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path
 */
DWORD WINAPI GetModulePathA(CHAR *pDirBuf, DWORD bufSize) {
    CHAR *szEnd = NULL;
    GetModuleFileNameA(NULL, pDirBuf, bufSize);
    szEnd = strrchr(pDirBuf, '\\');
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
    if (isHookRfid(rfid) && ppszPath) {
        WCHAR szDir[MAX_PATH] = { 0 };
        size_t length = GetModulePathW(szDir, MAX_PATH);
        PWSTR dirPath = CoTaskMemAlloc((length+1)*sizeof(TCHAR));
        if (dirPath == NULL) {
            return E_FAIL;
        }
        wcscpy(dirPath, szDir);
        *ppszPath = dirPath;
#ifdef HookDebug
        MessageBoxW(NULL, dirPath, (LPCWSTR) L"SHGetKnownFolderPath Hook Path", MB_OK);
#endif
        return S_OK;
    } else return OriginalSHGetKnownFolderPath(rfid, dwFlags, hToken, ppszPath);
}

typedef WINBOOL (WINAPI *pSHGetSpecialFolderPathA) (
        HWND   hwndOwner,
        _Out_ LPSTR  lpszPath,
        _In_  int    csidl,
        _In_  BOOL   fCreate
);

pSHGetSpecialFolderPathA OriginalSHGetSpecialFolderPathA = NULL;

WINBOOL WINAPI HookSHGetSpecialFolderPathA(
        HWND   hwndOwner,
        _Out_ LPSTR  lpszPath,
        _In_  int    csidl,
        _In_  BOOL   fCreate
) {
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && lpszPath) {
        if (lpszPath == NULL) {
            return FALSE;
        }
        GetModulePathA(lpszPath, MAX_PATH);
#ifdef HookDebug
        MessageBoxA(NULL, lpszPath, (LPCSTR) "SHGetSpecialFolderPath Hook Path", MB_OK);
#endif
        return TRUE;
    } else return OriginalSHGetSpecialFolderPathA(hwndOwner, lpszPath, csidl, fCreate);
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
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && lpszPath) {
        if (lpszPath == NULL) {
            return FALSE;
        }
        GetModulePathW(lpszPath, MAX_PATH);
#ifdef HookDebug
        MessageBoxW(NULL, lpszPath, (LPCWSTR) L"SHGetSpecialFolderPath Hook Path", MB_OK);
#endif
        return TRUE;
    } else return OriginalSHGetSpecialFolderPathW(hwndOwner, lpszPath, csidl, fCreate);
}

typedef HRESULT (WINAPI *pSHGetSpecialFolderLocation)(
  HWND             hwnd,
  int              csidl,
  PIDLIST_ABSOLUTE *ppidl
);

pSHGetSpecialFolderLocation OriginalSHGetSpecialFolderLocation = NULL;

HRESULT HookSHGetSpecialFolderLocation(
  HWND             hwnd,
  int              csidl,
  PIDLIST_ABSOLUTE *ppidl
) {
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && ppidl) {
        WCHAR lpszPath[MAX_PATH] = {0};
        GetModulePathW(lpszPath, MAX_PATH);
#ifdef HookDebug
        MessageBoxW(NULL, lpszPath, (LPCWSTR) L"SHGetSpecialFolderLocation Hook Path", MB_OK);
#endif
        return SHParseDisplayName(lpszPath, NULL, ppidl, SFGAO_FILESYSTEM, NULL);
    } else return OriginalSHGetSpecialFolderLocation(hwnd, csidl, ppidl);
}

typedef HRESULT (WINAPI *pSHGetKnownFolderIDList)(
  REFKNOWNFOLDERID rfid,
  DWORD            dwFlags,
  HANDLE           hToken,
  PIDLIST_ABSOLUTE *ppidl
);

pSHGetKnownFolderIDList OriginalSHGetKnownFolderIDList = NULL;

HRESULT HookSHGetKnownFolderIDList(
  REFKNOWNFOLDERID rfid,
  DWORD            dwFlags,
  HANDLE           hToken,
  PIDLIST_ABSOLUTE *ppidl
) {
    if (isHookRfid(rfid) && ppidl) {
        WCHAR lpszPath[MAX_PATH] = {0};
        GetModulePathW(lpszPath, MAX_PATH);
#ifdef HookDebug
        MessageBoxW(NULL, lpszPath, (LPCWSTR) L"SHGetKnownFolderIDList Hook Path", MB_OK);
#endif
        return SHParseDisplayName(lpszPath, NULL, ppidl, SFGAO_FILESYSTEM, NULL);
    } else return OriginalSHGetKnownFolderIDList(rfid, dwFlags, hToken, ppidl);

}

typedef HRESULT (WINAPI *pSHGetFolderPathAndSubDirW)(
  HWND    hwnd,
  int     csidl,
  HANDLE  hToken,
  DWORD   dwFlags,
  LPCWSTR pszSubDir,
  LPWSTR  pszPath
);

pSHGetFolderPathAndSubDirW OriginalSHGetFolderPathAndSubDirW = NULL;

HRESULT HookSHGetFolderPathAndSubDirW(
  HWND    hwnd,
  int     csidl,
  HANDLE  hToken,
  DWORD   dwFlags,
  LPCWSTR pszSubDir,
  LPWSTR  pszPath
) {
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && pszPath) {
        GetModulePathW(pszPath, MAX_PATH);
        if (pszSubDir) {
            wcscat_s(pszPath, MAX_PATH, L"\\");
            wcscat_s(pszPath, MAX_PATH, pszSubDir);
        }
#ifdef HookDebug
        MessageBoxW(NULL, pszPath, (LPCWSTR) L"SHGetFolderPathAndSubDirW Hook Path", MB_OK);
#endif
        if (csidl & CSIDL_FLAG_CREATE) {
            return CreateDirectoryW(pszPath, NULL) ? S_OK : S_FALSE;
        }
        return S_OK;
    } else return OriginalSHGetFolderPathAndSubDirW(hwnd, csidl, hToken, dwFlags, pszSubDir, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderPathAndSubDirA)(
  HWND    hwnd,
  int     csidl,
  HANDLE  hToken,
  DWORD   dwFlags,
  LPCSTR  pszSubDir,
  LPSTR   pszPath
);

pSHGetFolderPathAndSubDirA OriginalSHGetFolderPathAndSubDirA = NULL;

HRESULT HookSHGetFolderPathAndSubDirA(
  HWND    hwnd,
  int     csidl,
  HANDLE  hToken,
  DWORD   dwFlags,
  LPCSTR  pszSubDir,
  LPSTR   pszPath
) {
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && pszPath) {
        GetModulePathA(pszPath, MAX_PATH);
        if (pszSubDir) {
            strcat_s(pszPath, MAX_PATH, "\\");
            strcat_s(pszPath, MAX_PATH, pszSubDir);
        }
#ifdef HookDebug
        MessageBoxA(NULL, pszPath, (LPCSTR) "SHGetFolderPathAndSubDirA Hook Path", MB_OK);
#endif
        if (csidl & CSIDL_FLAG_CREATE) {
            return CreateDirectoryA(pszPath, NULL) ? S_OK : S_FALSE;
        }
        return S_OK;
    } else return OriginalSHGetFolderPathAndSubDirA(hwnd, csidl, hToken, dwFlags, pszSubDir, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderPathW)(
  HWND   hwnd,
  int    csidl,
  HANDLE hToken,
  DWORD  dwFlags,
  LPWSTR pszPath
);

pSHGetFolderPathW OriginalSHGetFolderPathW = NULL;

HRESULT HookSHGetFolderPathW(
  HWND   hwnd,
  int    csidl,
  HANDLE hToken,
  DWORD  dwFlags,
  LPWSTR pszPath
) {
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && pszPath) {
        GetModulePathW(pszPath, MAX_PATH);
#ifdef HookDebug
        MessageBoxW(NULL, pszPath, (LPCWSTR) L"SHGetFolderPathW Hook Path", MB_OK);
#endif
        return S_OK;
    } else return OriginalSHGetFolderPathW(hwnd, csidl, hToken, dwFlags, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderPathA)(
  HWND   hwnd,
  int    csidl,
  HANDLE hToken,
  DWORD  dwFlags,
  LPSTR  pszPath
);

pSHGetFolderPathA OriginalSHGetFolderPathA = NULL;

HRESULT HookSHGetFolderPathA(
  HWND   hwnd,
  int    csidl,
  HANDLE hToken,
  DWORD  dwFlags,
  LPSTR  pszPath
) {
    register int csidlLow = csidl & 0xff;
    if (isHookCsidl(csidlLow) && pszPath) {
        GetModulePathA(pszPath, MAX_PATH);
#ifdef HookDebug
        MessageBoxA(NULL, pszPath, (LPCSTR) "SHGetFolderPathW Hook Path", MB_OK);
#endif
        return S_OK;
    } else return OriginalSHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
}

void DLLHijackAttach(bool isSucceed) {
    if (isSucceed) {
        MH_Initialize();
#ifdef HookDebug
        MessageBox(NULL, TEXT("DLL Hijack Attach Succeed!"), TEXT(DLL_NAME " DLL Hijack Attach"), MB_OK);
#endif
        HMODULE shell32 = LoadLibrary((LPCTSTR) _T("shell32.dll"));
        void *SHGetKnownFolderPath = GetProcAddress(shell32, "SHGetKnownFolderPath");
        MH_STATUS status = CreateMinHook(SHGetKnownFolderPath);
        EnableMinHook(SHGetKnownFolderPath, status);

        void *SHGetSpecialFolderPathW = GetProcAddress(shell32, "SHGetSpecialFolderPathW");
        status = CreateMinHook(SHGetSpecialFolderPathW);
        EnableMinHook(SHGetSpecialFolderPathW, status);

        void *SHGetSpecialFolderPathA = GetProcAddress(shell32, "SHGetSpecialFolderPathA");
        status = CreateMinHook(SHGetSpecialFolderPathA);
        EnableMinHook(SHGetSpecialFolderPathA, status);

        void *SHGetSpecialFolderLocation = GetProcAddress(shell32, "SHGetSpecialFolderLocation");
        status = CreateMinHook(SHGetSpecialFolderLocation);
        EnableMinHook(SHGetSpecialFolderLocation, status);

        void *SHGetKnownFolderIDList = GetProcAddress(shell32, "SHGetKnownFolderIDList");
        status = CreateMinHook(SHGetKnownFolderIDList);
        EnableMinHook(SHGetKnownFolderIDList, status);

        void *SHGetFolderPathAndSubDirW = GetProcAddress(shell32, "SHGetFolderPathAndSubDirW");
        status = CreateMinHook(SHGetFolderPathAndSubDirW);
        EnableMinHook(SHGetFolderPathAndSubDirW, status);

        void *SHGetFolderPathAndSubDirA = GetProcAddress(shell32, "SHGetFolderPathAndSubDirA");
        status = CreateMinHook(SHGetFolderPathAndSubDirA);
        EnableMinHook(SHGetFolderPathAndSubDirA, status);

        void *SHGetFolderPathW = GetProcAddress(shell32, "SHGetFolderPathW");
        status = CreateMinHook(SHGetFolderPathW);
        EnableMinHook(SHGetFolderPathW, status);

        void *SHGetFolderPathA = GetProcAddress(shell32, "SHGetFolderPathA");
        status = CreateMinHook(SHGetFolderPathA);
        EnableMinHook(SHGetFolderPathA, status);

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
