#include <tchar.h>
#include <shlobj.h>
#include <stdbool.h>
#include <string.h>
#include "minhook/include/MinHook.h"

#ifdef _DEBUG
#define DebugMsg(format, ...) DbgPrintEx(-1, 0, format, ##__VA_ARGS__)
#else
#define DebugMsg(format, ...)
#endif

#define isHookCsidl(csidl)                                                     \
    ((csidl) == CSIDL_APPDATA ||                                               \
     (csidl) == CSIDL_PROGRAMS ||                                              \
     (csidl) == CSIDL_COMMON_APPDATA ||                                        \
     (csidl) == CSIDL_PERSONAL ||                                              \
     (csidl) == CSIDL_DESKTOP ||                                               \
     (csidl) == CSIDL_MYDOCUMENTS ||                                           \
     (csidl) == CSIDL_PROFILE ||                                               \
     (csidl) == CSIDL_COMMON_DOCUMENTS ||                                      \
     (csidl) == CSIDL_PROGRAM_FILES_COMMON ||                                  \
     (csidl) == CSIDL_LOCAL_APPDATA)

static boolean isHookRfid(const GUID *rfid) {
  return (IsEqualGUID(rfid, &FOLDERID_Programs) ||
          IsEqualGUID(rfid, &FOLDERID_LocalAppData) ||
          IsEqualGUID(rfid, &FOLDERID_LocalAppDataLow) ||
          IsEqualGUID(rfid, &FOLDERID_RoamingAppData) ||
          IsEqualGUID(rfid, &FOLDERID_Desktop) ||
          IsEqualGUID(rfid, &FOLDERID_Documents) ||
          IsEqualGUID(rfid, &FOLDERID_Profile) ||
          IsEqualGUID(rfid, &FOLDERID_ProgramFilesCommon) ||
          IsEqualGUID(rfid, &FOLDERID_ProgramFilesCommonX64) ||
          IsEqualGUID(rfid, &FOLDERID_ProgramFilesCommonX86) ||
          IsEqualGUID(rfid, &FOLDERID_ProgramFilesCommonX86) ||
          IsEqualGUID(rfid, &FOLDERID_Public) ||
          IsEqualGUID(rfid, &FOLDERID_PublicDesktop) ||
          IsEqualGUID(rfid, &FOLDERID_PublicDocuments));
}


static struct _ModulePathBuffer {
  size_t lengthA;
  size_t lengthW;
  char pathA[MAX_PATH];
  wchar_t pathW[MAX_PATH];
} paths = {0};

NTSYSAPI NTSTATUS RtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PCWCH UnicodeString,
    ULONG BytesInUnicodeString
);

boolean InitModulePath(void) {
  PTEB teb = NtCurrentTeb();
  // PEB->ImageBaseAddress
  HANDLE hModule = teb->ProcessEnvironmentBlock->Reserved3[1];
  PLDR_DATA_TABLE_ENTRY pEntry;
  if (!NT_SUCCESS(LdrFindEntryForAddress(hModule, &pEntry))) {
    return false;
  }
  size_t sizeW = min(sizeof(paths.pathW), pEntry->FullDllName.Length);
  memcpy(paths.pathW, pEntry->FullDllName.Buffer, sizeW);
  wchar_t *pathEnd = wcsrchr(paths.pathW, L'\\');
  if (pathEnd) {
    *pathEnd = 0;
    paths.lengthW = pathEnd - paths.pathW;
    sizeW = paths.lengthW * sizeof(wchar_t);
  } else {
    paths.lengthW = sizeW / sizeof(wchar_t);
  }
  ULONG sizeA = 0L;
  if (!NT_SUCCESS(RtlUnicodeToMultiByteN(paths.pathA, sizeof(paths.pathA),
                                         &sizeA, paths.pathW, sizeW))) {
    return false;
  }
  paths.lengthA = sizeA / sizeof(char);
  return true;
}

/**
 * GetModulePathA
 * @param {CHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path, 0 for failure
 */
DWORD WINAPI GetModulePathA(CHAR *pDirBuf, DWORD bufSize) {
  if (!pDirBuf || !bufSize) return 0;
  const DWORD size = min(bufSize, paths.lengthA);
  memcpy(pDirBuf, paths.pathA, size);
  return size;
}

/**
 * GetModulePathW
 * @param {WCHAR *} pDirBuf - destination buffer
 * @param {DWORD} bufSize - size of buffer
 * @return {DWORD} length of module path, 0 for failure
 */
DWORD WINAPI GetModulePathW(WCHAR *pDirBuf, DWORD bufSize) {
  if (!pDirBuf || !bufSize) return 0;
  const DWORD size = min(bufSize, paths.lengthW * sizeof(wchar_t));
  memcpy(pDirBuf, paths.pathW, size);
  return size / sizeof(wchar_t);
}

NTSYSAPI NTSTATUS RtlMultiByteToUnicodeN(
    PWCH UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    const CHAR *MultiByteString,
    ULONG BytesInMultiByteString
);

bool
NtCreateDirectoryW(PCWSTR pszFileName) {
  NTSTATUS Status;
  UNICODE_STRING FileName;
  HANDLE DirectoryHandle;
  IO_STATUS_BLOCK IoStatus;
  OBJECT_ATTRIBUTES ObjectAttributes;

  RtlInitUnicodeString(&FileName, pszFileName);
  InitializeObjectAttributes(&ObjectAttributes, &FileName, 0, NULL, NULL);

  Status = NtCreateFile(&DirectoryHandle,
                        GENERIC_READ | GENERIC_WRITE,
                        &ObjectAttributes,
                        &IoStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_CREATE,
                        FILE_DIRECTORY_FILE,
                        NULL,
                        0);

  if (NT_SUCCESS(Status)) {
    NtClose(DirectoryHandle);
    return true;
  }
  return false;
}

bool
NtCreateDirectoryA(PCSTR pszFileName) {
  WCHAR buffer[MAX_PATH];
  ULONG outLen = 0;
  if (!NT_SUCCESS(RtlMultiByteToUnicodeN(buffer, MAX_PATH, &outLen, pszFileName, strlen(pszFileName)))) {
    return false;
  }
  return NtCreateDirectoryW(buffer);
}

void *LdrGetDllFunction(const wchar_t *dll, const char *fn) {
  UNICODE_STRING dllu;
  RtlInitUnicodeString(&dllu, dll);

  PVOID handle;
  NTSTATUS status = LdrGetDllHandle(NULL, NULL, &dllu, &handle);

  if (!NT_SUCCESS(status))
    status = LdrLoadDll(NULL, NULL, &dllu, &handle);

  if (!NT_SUCCESS(status))
    return NULL;

  ANSI_STRING fna;
  RtlInitAnsiString(&fna, fn);

  PVOID proc;
  status = LdrGetProcedureAddress(handle, &fna, 0, &proc);

  return NT_SUCCESS(status) ? proc : NULL;
}

typedef LPVOID (WINAPI *pCoTaskMemAlloc)(SIZE_T cb);

pCoTaskMemAlloc pfnCoTaskMemAlloc = NULL;

typedef HRESULT (WINAPI *pSHParseDisplayName)(
    PCWSTR pszName,
    IBindCtx *pbc,
    PIDLIST_ABSOLUTE *ppidl,
    SFGAOF sfgaoIn,
    SFGAOF *psfgaoOut);

pSHParseDisplayName pfnSHParseDisplayName = NULL;

typedef struct _HookTable {
  void *original;
  void *hook;
  char *name;
} HookTableSt;
static HookTableSt HookTable[];

#define DefineHook(name) name ## Index
enum _HookTableIndex {
  DefineHook(SHGetKnownFolderPath) = 0,
  DefineHook(SHGetSpecialFolderPathW),
  DefineHook(SHGetSpecialFolderPathA),
  DefineHook(SHGetSpecialFolderLocation),
  DefineHook(SHGetKnownFolderIDList),
  DefineHook(SHGetFolderPathAndSubDirW),
  DefineHook(SHGetFolderPathAndSubDirA),
  DefineHook(SHGetFolderPathW),
  DefineHook(SHGetFolderPathA),
  DefineHook(SHGetFolderLocation),
  DefineHook(SHGetFolderPathEx),
  _HookLength
};
#undef DefineHook
#define OriginalFunc(func) ((p ## func)(HookTable[func ## Index].original))

typedef HRESULT (WINAPI *pSHGetKnownFolderPath)(
    _In_     REFKNOWNFOLDERID rfid,
    _In_     DWORD dwFlags,
    _In_opt_ HANDLE hToken,
    _Out_    PWSTR *ppszPath
);

HRESULT WINAPI HookSHGetKnownFolderPath(
    _In_     REFKNOWNFOLDERID rfid,
    _In_     DWORD dwFlags,
    _In_opt_ HANDLE hToken,
    _Out_    PWSTR *ppszPath
) {
  if (isHookRfid(rfid) && ppszPath) {
    WCHAR szDir[MAX_PATH] = {0};
    size_t length = GetModulePathW(szDir, MAX_PATH);

    if (!pfnCoTaskMemAlloc) {
      pfnCoTaskMemAlloc = LdrGetDllFunction(L"ole32.dll", "CoTaskMemAlloc");
    }
    if (!pfnCoTaskMemAlloc) {
      return E_FAIL;
    }
    PWSTR dirPath = pfnCoTaskMemAlloc((length + 1) * sizeof(TCHAR));
    if (dirPath == NULL) {
      return E_FAIL;
    }
    wcscpy(dirPath, szDir);
    *ppszPath = dirPath;
    DebugMsg("SHGetKnownFolderPath Hook Path: %ls", dirPath);
    return S_OK;
  } else return OriginalFunc(SHGetKnownFolderPath)(rfid, dwFlags, hToken, ppszPath);
}

typedef WINBOOL (WINAPI *pSHGetSpecialFolderPathA)(
    HWND hwndOwner,
    _Out_ LPSTR lpszPath,
    _In_  int csidl,
    _In_  BOOL fCreate
);

WINBOOL WINAPI HookSHGetSpecialFolderPathA(
    HWND hwndOwner,
    _Out_ LPSTR lpszPath,
    _In_  int csidl,
    _In_  BOOL fCreate
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && lpszPath) {
    GetModulePathA(lpszPath, MAX_PATH);
    DebugMsg("SHGetSpecialFolderPathA Hook Path: %s", lpszPath);
    return TRUE;
  } else return OriginalFunc(SHGetSpecialFolderPathA)(hwndOwner, lpszPath, csidl, fCreate);
}

typedef WINBOOL (WINAPI *pSHGetSpecialFolderPathW)(
    HWND hwndOwner,
    _Out_ LPWSTR lpszPath,
    _In_  int csidl,
    _In_  BOOL fCreate
);

WINBOOL WINAPI HookSHGetSpecialFolderPathW(
    HWND hwndOwner,
    _Out_ LPWSTR lpszPath,
    _In_  int csidl,
    _In_  BOOL fCreate
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && lpszPath) {
    GetModulePathW(lpszPath, MAX_PATH);
    DebugMsg("SHGetSpecialFolderPathW Hook Path: %ls", lpszPath);
    return TRUE;
  } else return OriginalFunc(SHGetSpecialFolderPathW)(hwndOwner, lpszPath, csidl, fCreate);
}

typedef HRESULT (WINAPI *pSHGetSpecialFolderLocation)(
    HWND hwnd,
    int csidl,
    PIDLIST_ABSOLUTE *ppidl
);

HRESULT HookSHGetSpecialFolderLocation(
    HWND hwnd,
    int csidl,
    PIDLIST_ABSOLUTE *ppidl
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && ppidl) {
    WCHAR lpszPath[MAX_PATH] = {0};
    GetModulePathW(lpszPath, MAX_PATH);
    DebugMsg("SHGetSpecialFolderLocation Hook Path: %ls", lpszPath);
    if (!pfnSHParseDisplayName) {
      pfnSHParseDisplayName = LdrGetDllFunction(L"shell32.dll", "SHParseDisplayName");
    }
    if (!pfnSHParseDisplayName) {
      return E_FAIL;
    }
    return pfnSHParseDisplayName(lpszPath, NULL, ppidl, SFGAO_FILESYSTEM, NULL);
  } else return OriginalFunc(SHGetSpecialFolderLocation)(hwnd, csidl, ppidl);
}

typedef HRESULT (WINAPI *pSHGetKnownFolderIDList)(
    REFKNOWNFOLDERID rfid,
    DWORD dwFlags,
    HANDLE hToken,
    PIDLIST_ABSOLUTE *ppidl
);

HRESULT HookSHGetKnownFolderIDList(
    REFKNOWNFOLDERID rfid,
    DWORD dwFlags,
    HANDLE hToken,
    PIDLIST_ABSOLUTE *ppidl
) {
  if (isHookRfid(rfid) && ppidl) {
    WCHAR lpszPath[MAX_PATH] = {0};
    GetModulePathW(lpszPath, MAX_PATH);
    DebugMsg("SHGetKnownFolderIDList Hook Path: %ls", lpszPath);
    if (!pfnSHParseDisplayName) {
      pfnSHParseDisplayName = LdrGetDllFunction(L"shell32.dll", "SHParseDisplayName");
    }
    if (!pfnSHParseDisplayName) {
      return E_FAIL;
    }
    return pfnSHParseDisplayName(lpszPath, NULL, ppidl, SFGAO_FILESYSTEM, NULL);
  } else return OriginalFunc(SHGetKnownFolderIDList)(rfid, dwFlags, hToken, ppidl);
}

typedef HRESULT (WINAPI *pSHGetFolderPathAndSubDirW)(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPCWSTR pszSubDir,
    LPWSTR pszPath
);

HRESULT HookSHGetFolderPathAndSubDirW(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPCWSTR pszSubDir,
    LPWSTR pszPath
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && pszPath) {
    GetModulePathW(pszPath, MAX_PATH);
    if (pszSubDir) {
      wcscat_s(pszPath, MAX_PATH, L"\\");
      wcscat_s(pszPath, MAX_PATH, pszSubDir);
      if (csidl & CSIDL_FLAG_CREATE) {
        return NtCreateDirectoryW(pszPath) ? S_OK : S_FALSE;
      }
    }
    DebugMsg("SHGetFolderPathAndSubDirW Hook Path: %ls", pszPath);
    return S_OK;
  } else return OriginalFunc(SHGetFolderPathAndSubDirW)(hwnd, csidl, hToken, dwFlags, pszSubDir, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderPathAndSubDirA)(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPCSTR pszSubDir,
    LPSTR pszPath
);

HRESULT HookSHGetFolderPathAndSubDirA(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPCSTR pszSubDir,
    LPSTR pszPath
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && pszPath) {
    GetModulePathA(pszPath, MAX_PATH);
    if (pszSubDir) {
      strcat_s(pszPath, MAX_PATH, "\\");
      strcat_s(pszPath, MAX_PATH, pszSubDir);
      if (csidl & CSIDL_FLAG_CREATE) {
        return NtCreateDirectoryA(pszPath) ? S_OK : S_FALSE;
      }
    }
    DebugMsg("SHGetFolderPathAndSubDirW Hook Path: %s", pszPath);
    return S_OK;
  } else return OriginalFunc(SHGetFolderPathAndSubDirA)(hwnd, csidl, hToken, dwFlags, pszSubDir, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderPathW)(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPWSTR pszPath
);

HRESULT HookSHGetFolderPathW(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPWSTR pszPath
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && pszPath) {
    GetModulePathW(pszPath, MAX_PATH);
    DebugMsg("SHGetFolderPathW Hook Path: %ls", pszPath);
    return S_OK;
  } else return OriginalFunc(SHGetFolderPathW)(hwnd, csidl, hToken, dwFlags, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderPathA)(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPSTR pszPath
);

HRESULT HookSHGetFolderPathA(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPSTR pszPath
) {
  register int csidlLow = csidl & 0xff;
  if (isHookCsidl(csidlLow) && pszPath) {
    GetModulePathA(pszPath, MAX_PATH);
    DebugMsg("SHGetFolderPathA Hook Path: %s", pszPath);
    return S_OK;
  } else return OriginalFunc(SHGetFolderPathA)(hwnd, csidl, hToken, dwFlags, pszPath);
}

typedef HRESULT (WINAPI *pSHGetFolderLocation)(
    _In_        HWND hwndOwner,
    _In_        int nFolder,
    _In_        HANDLE hToken,
    _Reserved_  DWORD dwReserved,
    _Out_       PIDLIST_ABSOLUTE *ppidl
);


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
    DebugMsg("SHGetFolderLocation Hook Path: %ls", lpszPath);
    if (!pfnSHParseDisplayName) {
      pfnSHParseDisplayName = LdrGetDllFunction(L"shell32.dll", "SHParseDisplayName");
    }
    if (!pfnSHParseDisplayName) {
      return E_FAIL;
    }
    return pfnSHParseDisplayName(lpszPath, NULL, ppidl, SFGAO_FILESYSTEM, NULL);
  } else return OriginalFunc(SHGetFolderLocation)(hwndOwner, nFolder, hToken, dwReserved, ppidl);
}

typedef HRESULT (WINAPI *pSHGetFolderPathEx)(
    _In_     REFKNOWNFOLDERID rfid,
    _In_     DWORD dwFlags,
    _In_opt_ HANDLE hToken,
    _Out_    LPWSTR pszPath,
    _In_     UINT cchPath
);

HRESULT HookSHGetFolderPathEx(
    _In_     REFKNOWNFOLDERID rfid,
    _In_     DWORD dwFlags,
    _In_opt_ HANDLE hToken,
    _Out_    LPWSTR pszPath,
    _In_     UINT cchPath
) {
  if (isHookRfid(rfid) && pszPath) {
    GetModulePathW(pszPath, cchPath);
    DebugMsg("SHGetFolderPathEx Hook Path: %ls", pszPath);
    return S_OK;
  } else return OriginalFunc(SHGetFolderPathEx)(rfid, dwFlags, hToken, pszPath, cchPath);
}

#define DefineHook(name) {NULL, Hook ## name, #name}
static HookTableSt HookTable[] = {
    DefineHook(SHGetKnownFolderPath),
    DefineHook(SHGetSpecialFolderPathW),
    DefineHook(SHGetSpecialFolderPathA),
    DefineHook(SHGetSpecialFolderLocation),
    DefineHook(SHGetKnownFolderIDList),
    DefineHook(SHGetFolderPathAndSubDirW),
    DefineHook(SHGetFolderPathAndSubDirA),
    DefineHook(SHGetFolderPathW),
    DefineHook(SHGetFolderPathA),
    DefineHook(SHGetFolderLocation),
    DefineHook(SHGetFolderPathEx)
};
#undef DefineHook

void DLLHijackAttach(bool isSucceed) {
  if (isSucceed && InitModulePath()) {
    MH_STATUS initStatus = MH_Initialize();
    DebugMsg(DLL_NAME " DLL Hijack Attach Succeed");
    if (initStatus != MH_OK) {
      DebugMsg("MinHook init failed: %d", initStatus);
      return;
    }
    HMODULE shell32 = LdrLoadLibraryW((LPCWSTR) L"shell32.dll");
    for (unsigned i = 0; i < _HookLength; ++i) {
      void *func = LdrGetProcAddressA(shell32, HookTable[i].name);
      if (!func) {
        DebugMsg("Hook at %s failed: original func not found", HookTable[i].name);
        continue;
      }
      MH_STATUS status = MH_CreateHook(func, HookTable[i].hook, HookTable[i].original);
      if (status != MH_OK) {
        DebugMsg("Hook at %s failed: MH_CreateHook fail %d", HookTable[i].name, status);
        continue;
      }
      status = MH_EnableHook(func);
      if (status != MH_OK) {
        DebugMsg("Hook at %s failed: MH_EnableHook fail %d", HookTable[i].name, status);
      }
    }
  }
}

void DLLHijackDetach(bool isSucceed) {
  if (isSucceed) {
    MH_Uninitialize();
    DebugMsg(DLL_NAME " DLL Hijack DetachSucceed");
  }
}
