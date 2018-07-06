#include <tchar.h>
#include <shlobj.h>
#include <stdbool.h>
#include <string.h>
#include "minhook/include/MinHook.h"

#define CreateMinHook(func)                                                    \
    MH_CreateHook(func, &Hook##func, (LPVOID *)&Original##func)

#ifdef HookDebug

#define HookMsg(title, content)                                                \
    MessageBox(NULL, (LPCTSTR)_T(content), (LPCTSTR)_T(title), MB_OK)
#define HookMsgA(title, content)                                               \
    MessageBoxA(NULL, (LPCSTR)(content), (LPCSTR)(title), MB_OK)
#define HookMsgW(title, content)                                               \
    MessageBoxW(NULL, (LPCWSTR)(content), (LPCWSTR)(title), MB_OK);

#define EnableMinHook(func, status)                                            \
    if ((status) == MH_OK) {                                                   \
        (status) = MH_EnableHook(func);                                        \
        if ((status) == MH_OK) {                                               \
            HookMsg((#func " Hook"), (#func " Hook Succeed"));                 \
        } else {                                                               \
            HookMsg((#func " Hook"), (#func " Hook Enable Fail"));             \
        }                                                                      \
    } else {                                                                   \
        HookMsg(#func " Hook Fail", #func " Hook");                            \
    }

#define DoMinHook(library, function)                                           \
    {                                                                          \
        void *function = GetProcAddress(library, #function);                   \
        if (function) {                                                        \
            MH_STATUS status = CreateMinHook(function);                        \
            EnableMinHook(function, status);                                   \
        } else {                                                               \
            HookMsg(#function, "Cannot get address of function");              \
        }                                                                      \
    }

#else // !HookDebug

#define HookMsg(title, content)
#define HookMsgA(title, content)
#define HookMsgW(title, content)

#define EnableMinHook(func, status)                                            \
    if ((status) == MH_OK) {                                                   \
        MH_EnableHook(func);                                                   \
    }

#define DoMinHook(library, function)                                           \
    {                                                                          \
        void *(function) = GetProcAddress(library, #function);                 \
        if (function) {                                                        \
            MH_STATUS status = CreateMinHook(function);                        \
            EnableMinHook(function, status);                                   \
        }                                                                      \
    }

#endif // HookDebug

#define isHookCsidl(csidl)                                                     \
    ((csidl) == CSIDL_APPDATA ||                                               \
     (csidl) == CSIDL_PROGRAMS ||                                              \
     (csidl) == CSIDL_COMMON_APPDATA ||                                        \
     (csidl) == CSIDL_PERSONAL ||                                              \
     (csidl) == CSIDL_DESKTOP ||                                               \
     (csidl) == CSIDL_MYDOCUMENTS ||                                           \
     (csidl) == CSIDL_PROFILE ||                                               \
     (csidl) == CSIDL_COMMON_DOCUMENTS ||                                      \
     (csidl) == CSIDL_LOCAL_APPDATA)

#define isHookRfid(rfid)                                                       \
    (IsEqualGUID(rfid, &FOLDERID_Programs) ||                                  \
     IsEqualGUID(rfid, &FOLDERID_LocalAppData) ||                              \
     IsEqualGUID(rfid, &FOLDERID_Desktop) ||                                   \
     IsEqualGUID(rfid, &FOLDERID_Documents) ||                                 \
     IsEqualGUID(rfid, &FOLDERID_LocalAppDataLow) ||                           \
     IsEqualGUID(rfid, &FOLDERID_Profile) ||                                   \
     IsEqualGUID(rfid, &FOLDERID_RoamingAppData))

/**
 * GetModulePath
 * @param {TCHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path, 0 for failure
 */
DWORD WINAPI GetModulePath(TCHAR *pDirBuf, DWORD bufSize) {
    TCHAR *szEnd = NULL;
    GetModuleFileName(NULL, pDirBuf, bufSize);
    szEnd = _tcsrchr(pDirBuf, _T('\\'));
    if (szEnd) {
        *(szEnd) = 0;
        return szEnd - pDirBuf;
    }
    return 0;
}

/**
 * GetModulePathA
 * @param {CHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path, 0 for failure
 */
DWORD WINAPI GetModulePathA(CHAR *pDirBuf, DWORD bufSize) {
    CHAR *szEnd = NULL;
    GetModuleFileNameA(NULL, pDirBuf, bufSize);
    szEnd = strrchr(pDirBuf, '\\');
    if (szEnd) {
        *(szEnd) = 0;
        return szEnd - pDirBuf;
    }
    return 0;
}

/**
 * GetModulePathW
 * @param {WCHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path, 0 for failure
 */
DWORD WINAPI GetModulePathW(WCHAR *pDirBuf, DWORD bufSize) {
    WCHAR *szEnd = NULL;
    GetModuleFileNameW(NULL, pDirBuf, bufSize);
    szEnd = wcsrchr(pDirBuf, L'\\');
    if (szEnd) {
        *(szEnd) = 0;
        return szEnd - pDirBuf;
    }
    return 0;
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
        HookMsgW(L"SHGetKnownFolderPath Hook Path", dirPath);
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
        GetModulePathA(lpszPath, MAX_PATH);
        HookMsgA("SHGetSpecialFolderPathA Hook Path", lpszPath);
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
        GetModulePathW(lpszPath, MAX_PATH);
        HookMsgW(L"SHGetSpecialFolderPathW Hook Path", lpszPath);
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
        HookMsgW(L"SHGetSpecialFolderLocation Hook Path", lpszPath);
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
        HookMsgW(L"SHGetKnownFolderIDList Hook Path", lpszPath);
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
            if (csidl & CSIDL_FLAG_CREATE) {
                return CreateDirectoryW(pszPath, NULL) ? S_OK : S_FALSE;
            }
        }
        HookMsgW(L"SHGetFolderPathAndSubDirW Hook Path", pszPath);
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
            if (csidl & CSIDL_FLAG_CREATE) {
                return CreateDirectoryA(pszPath, NULL) ? S_OK : S_FALSE;
            }
        }
        HookMsgA("SHGetFolderPathAndSubDirW Hook Path", pszPath);
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
        HookMsgW(L"SHGetFolderPathW Hook Path", pszPath);
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
        HookMsgA("SHGetFolderPathA Hook Path", pszPath);
        return S_OK;
    } else return OriginalSHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderLocation)(
  _In_        HWND hwndOwner,
  _In_        int nFolder,
  _In_        HANDLE hToken,
  _Reserved_  DWORD dwReserved,
  _Out_       PIDLIST_ABSOLUTE *ppidl
);

pSHGetFolderLocation OriginalSHGetFolderLocation = NULL;

HRESULT HookSHGetFolderLocation(
  _In_        HWND hwndOwner,
  _In_        int nFolder,
  _In_        HANDLE hToken,
  _Reserved_  DWORD dwReserved,
  _Out_       PIDLIST_ABSOLUTE *ppidl
) {
    register int csidlLow = nFolder & 0xff;
    if (isHookCsidl(csidlLow) && ppidl) {
        WCHAR lpszPath[MAX_PATH] = {0};
        GetModulePathW(lpszPath, MAX_PATH);
        HookMsgW("SHGetFolderLocation Hook Path", lpszPath);
        return SHParseDisplayName(lpszPath, NULL, ppidl, SFGAO_FILESYSTEM, NULL);
    } else return OriginalSHGetFolderLocation(hwndOwner, nFolder, hToken, dwReserved, ppidl);
}

typedef HRESULT (WINAPI *pSHGetFolderPathEx)(
  _In_     REFKNOWNFOLDERID rfid,
  _In_     DWORD            dwFlags,
  _In_opt_ HANDLE           hToken,
  _Out_    LPWSTR           pszPath,
  _In_     UINT             cchPath
);

pSHGetFolderPathEx OriginalSHGetFolderPathEx = NULL;

HRESULT HookSHGetFolderPathEx(
  _In_     REFKNOWNFOLDERID rfid,
  _In_     DWORD            dwFlags,
  _In_opt_ HANDLE           hToken,
  _Out_    LPWSTR           pszPath,
  _In_     UINT             cchPath
) {
    if (isHookRfid(rfid) && pszPath) {
        GetModulePathW(pszPath, cchPath);
        HookMsgW("SHGetFolderPathEx Hook Path", pszPath);
        return S_OK;
    } else return OriginalSHGetFolderPathEx(rfid, dwFlags, hToken, pszPath, cchPath);
}

void DLLHijackAttach(bool isSucceed) {
    if (isSucceed) {
        MH_Initialize();
        HookMsg((DLL_NAME " DLL Hijack Attach"), "DLL Hijack Attach Succeed!");
        HMODULE shell32 = LoadLibrary((LPCTSTR) _T("shell32.dll"));

        DoMinHook(shell32, SHGetKnownFolderPath);
        DoMinHook(shell32, SHGetSpecialFolderPathW);
        DoMinHook(shell32, SHGetSpecialFolderPathA);
        DoMinHook(shell32, SHGetSpecialFolderLocation);
        DoMinHook(shell32, SHGetKnownFolderIDList);
        DoMinHook(shell32, SHGetFolderPathAndSubDirW);
        DoMinHook(shell32, SHGetFolderPathAndSubDirA);
        DoMinHook(shell32, SHGetFolderPathW);
        DoMinHook(shell32, SHGetFolderPathA);
        DoMinHook(shell32, SHGetFolderLocation);
        DoMinHook(shell32, SHGetFolderPathEx);
    }
}

void DLLHijackDetach(bool isSucceed) {
    if (isSucceed) {
        MH_Uninitialize();
        HookMsg((DLL_NAME " DLL Hijack Detach"), "DLL Hijack Detach Succeed!");
    }
}
