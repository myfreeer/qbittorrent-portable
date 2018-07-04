// linker flags: -lshell32 -lversion -luuid -lole32
#ifndef UNICODE
#define UNICODE
#endif // !UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // !_UNICODE

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>

#define showInfopW(name, path) \
    printf(name ": %ls\n", path); \
    CoTaskMemFree(path);

#define showInfoW(name) \
    printf(name ": %ls\n", pathW); \
    ZeroMemory(pathW, MAX_PATH * sizeof(WCHAR));

#define showInfoA(name) \
    printf(name ": %s\n", pathA); \
    ZeroMemory(pathA, MAX_PATH * sizeof(char));

STDAPI SHGetKnownFolderPath (REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
STDAPI SHGetKnownFolderIDList (REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PIDLIST_ABSOLUTE *ppidl);
HRESULT WINAPI SHGetFolderPathEx(
  _In_     REFKNOWNFOLDERID rfid,
  _In_     DWORD            dwFlags,
  _In_opt_ HANDLE           hToken,
  _Out_    LPWSTR           pszPath,
  _In_     UINT             cchPath
);

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

int mainCRTStartup() {
    WCHAR versionDllW[MAX_PATH] = {0};
    GetSystemDirectory(versionDllW, MAX_PATH);
    lstrcat(versionDllW, L"\\version.dll");
    LPDWORD lpdwHandle = NULL;
    DWORD size = GetFileVersionInfoSizeW(versionDllW, lpdwHandle);
    printf("GetFileVersionInfoSizeW: %lu\n", size);

    WCHAR modulePathW[MAX_PATH] = {0};
    GetModulePathW(modulePathW, MAX_PATH);
    printf("GetModulePathW: %ls\n", modulePathW);

    char modulePathA[MAX_PATH] = {0};
    GetModulePathA(modulePathA, MAX_PATH);
    printf("GetModulePathA: %s\n", modulePathA);

    WCHAR pathW[MAX_PATH] = {0};
    char pathA[MAX_PATH] = {0};

    SHGetSpecialFolderPathW(NULL, pathW, CSIDL_APPDATA, FALSE);
    showInfoW("SHGetSpecialFolderPathW");
    SHGetSpecialFolderPathA(NULL, pathA, CSIDL_APPDATA, FALSE);
    showInfoA("SHGetSpecialFolderPathA");

    LPITEMIDLIST pidl = 0;
    SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
    SHGetPathFromIDListW(pidl, pathW);
    showInfoW("SHGetSpecialFolderLocation");
    SHGetPathFromIDListA(pidl, pathA);
    showInfoA("SHGetSpecialFolderLocation");
    ILFree(pidl);
    pidl = 0;

    WCHAR *ppathW = NULL;
    SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, NULL, &ppathW);
    showInfopW("SHGetKnownFolderPath", ppathW);

    SHGetKnownFolderIDList(&FOLDERID_RoamingAppData, 0, NULL, &pidl);
    SHGetPathFromIDListW(pidl, pathW);
    showInfoW("SHGetKnownFolderIDList");
    SHGetPathFromIDListA(pidl, pathA);
    showInfoA("SHGetKnownFolderIDList");
    ILFree(pidl);
    pidl = 0;

    SHGetFolderPathAndSubDirW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, NULL, pathW);
    showInfoW("SHGetFolderPathAndSubDirW");
    SHGetFolderPathAndSubDirA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, NULL, pathA);
    showInfoA("SHGetFolderPathAndSubDirA");

    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathW);
    showInfoW("SHGetFolderPathW");
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathA);
    showInfoA("SHGetFolderPathA");

    SHGetFolderLocation(NULL, CSIDL_APPDATA, NULL, 0, &pidl);
    SHGetPathFromIDListW(pidl, pathW);
    showInfoW("SHGetFolderLocation");
    SHGetPathFromIDListA(pidl, pathA);
    showInfoA("SHGetFolderLocation");
    ILFree(pidl);
    pidl = 0;

    SHGetFolderPathEx(&FOLDERID_RoamingAppData, 0, NULL, pathW, MAX_PATH);
    showInfoW("SHGetFolderPathEx");

    ExitProcess(0);
    return 0;
}