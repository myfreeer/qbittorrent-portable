// linker flags: -lshell32 -lversion -luuid -lole32
#ifndef UNICODE
#define UNICODE
#endif // !UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // !_UNICODE

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <shlobj.h>

// ---------------------------------------------------------------------------
// ABI / calling-convention guard.
//
// The shell32 folder APIs below are __stdcall (WINAPI). They are hooked (via
// the colocated version.dll hijack shim, loaded at process init) with MinHook,
// which overwrites the target prologue with a jmp to a detour. If a detour is
// declared with the wrong calling convention (e.g. __cdecl instead of WINAPI),
// x86 code generation diverges on *who cleans the argument bytes off the
// stack*: __stdcall expects the callee to `ret N`, __cdecl expects the caller
// to clean up. A mismatched detour therefore leaves ESP off by the argument
// byte count on every call. That imbalance is silent here (our frames use EBP
// and never re-enter), but it corrupts the stack inside shell32's FPO dialog
// code -> the real qBittorrent crash on opening a file-selection dialog.
//
// This guard reads ESP immediately before and after each hooked call. For a
// __stdcall-declared call the compiler pushes the args and expects the callee
// to reclaim them, so:
//   - correct callee (WINAPI detour): ESP unchanged   -> delta 0, pass
//   - cdecl detour called as stdcall: ESP low by argN  -> delta != 0, FAIL
// On x64 there is a single calling convention, so this compiles to a no-op
// that always passes.
// ---------------------------------------------------------------------------
#if defined(__i386__) || defined(_M_IX86)
  #if defined(__GNUC__)
    #define READ_SP(v) __asm__ __volatile__("movl %%esp, %0" : "=r"(v) : : "memory")
  #elif defined(_MSC_VER)
    #define READ_SP(v) do { __asm { mov v, esp } } while (0)
  #else
    #error "unsupported x86 compiler for ESP probe"
  #endif
  #define ABI_PROBE 1
#else
  // x64: one calling convention; nothing to verify.
  #define READ_SP(v) ((v) = (uintptr_t)0)
  #define ABI_PROBE 0
#endif

static int g_failures = 0;
// Number of hooked calls that were observably redirected (returned the module
// directory instead of the real shell folder). If this stays 0 the hijack shim
// never installed the hooks, so the ABI probe ran against unhooked shell32 and
// its "pass" would be meaningless -- treat that as a hard failure.
static int g_hooked = 0;

// Bracket a hooked call and verify it left the stack balanced. `label` is a
// string; `callexpr` is the full call statement (may assign a result).
#define CHECK_ABI(label, callexpr)                                             \
    do {                                                                       \
        uintptr_t __sp0, __sp1;                                                \
        READ_SP(__sp0);                                                        \
        callexpr;                                                              \
        READ_SP(__sp1);                                                        \
        if (__sp0 != __sp1) {                                                  \
            printf("FAIL: %s left ESP off by %ld byte(s) (ABI mismatch)\n",    \
                   label, (long)((intptr_t)__sp0 - (intptr_t)__sp1));          \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

// Generic behavioral assertion (return values, path content, pass-through).
#define CHECK(label, cond)                                                     \
    do {                                                                       \
        if (!(cond)) {                                                         \
            printf("FAIL: %s\n", label);                                       \
            g_failures++;                                                      \
        }                                                                      \
    } while (0)

// Liveness gate: a redirected call returns the module directory. A hit proves
// the hook actually ran; if nothing redirects, the ABI probe is meaningless.
#define HOOK_LIVE_W(name, got)                                                 \
    do { if (wcscmp((got), modulePathW) == 0) g_hooked++;                      \
         else printf("NOTE: %s not redirected (got %ls)\n", name, (got)); } while (0)
#define HOOK_LIVE_A(name, got)                                                 \
    do { if (strcmp((got), modulePathA) == 0) g_hooked++;                      \
         else printf("NOTE: %s not redirected (got %s)\n", name, (got)); } while (0)
// AndSubDir variants return "<module dir>\<subdir>", so match the prefix only.
#define HOOK_LIVE_W_PREFIX(name, got)                                          \
    do { if (wcsncmp((got), modulePathW, wcslen(modulePathW)) == 0) g_hooked++;\
         else printf("NOTE: %s not redirected (got %ls)\n", name, (got)); } while (0)
#define HOOK_LIVE_A_PREFIX(name, got)                                          \
    do { if (strncmp((got), modulePathA, strlen(modulePathA)) == 0) g_hooked++;\
         else printf("NOTE: %s not redirected (got %s)\n", name, (got)); } while (0)

#define showInfopW(name, path) \
    printf(name ": %ls\n", path); \
    CoTaskMemFree(path);

#define showInfoW(name) \
    printf(name ": %ls\n", pathW); \
    ZeroMemory(pathW, MAX_PATH * sizeof(WCHAR));

#define showInfoA(name) \
    printf(name ": %s\n", pathA); \
    ZeroMemory(pathA, MAX_PATH * sizeof(char));

// Convert a wide string to ANSI (CP_ACP) for A/W consistency comparisons.
static const char *toAnsi(const WCHAR *w, char *buf, int cb) {
    WideCharToMultiByte(CP_ACP, 0, w, -1, buf, cb, NULL, NULL);
    return buf;
}

STDAPI SHGetKnownFolderPath (REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
STDAPI SHGetKnownFolderIDList (REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken, PIDLIST_ABSOLUTE *ppidl);
HRESULT WINAPI SHGetFolderPathEx(
  _In_     REFKNOWNFOLDERID rfid,
  _In_     DWORD            dwFlags,
  _In_opt_ HANDLE           hToken,
  _Out_    LPWSTR           pszPath,
  _In_     UINT             cchPath
);

int __cdecl atexit(void (__cdecl * unused)(void)) {
    (void) unused;
    return 0;
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

    printf("ABI probe: %s\n", ABI_PROBE ? "enabled (x86)" : "disabled (x64, single convention)");

    // The two module-path helpers convert independently; the redirected results
    // are only comparable across the A/W hooks if these agree to begin with.
    {
        char __w2a[MAX_PATH] = {0};
        CHECK("module path A/W consistent",
              strcmp(toAnsi(modulePathW, __w2a, sizeof(__w2a)), modulePathA) == 0);
    }

    WCHAR pathW[MAX_PATH] = {0};
    char pathA[MAX_PATH] = {0};
    HRESULT hr;
    BOOL ok;

    // --- SHGetSpecialFolderPathW / A -------------------------------------
    CHECK_ABI("SHGetSpecialFolderPathW",
              ok = SHGetSpecialFolderPathW(NULL, pathW, CSIDL_APPDATA, FALSE));
    CHECK("SHGetSpecialFolderPathW returns TRUE", ok);
    HOOK_LIVE_W("SHGetSpecialFolderPathW", pathW);
    CHECK("SHGetSpecialFolderPathW == module dir", wcscmp(pathW, modulePathW) == 0);
    showInfoW("SHGetSpecialFolderPathW");

    CHECK_ABI("SHGetSpecialFolderPathA",
              ok = SHGetSpecialFolderPathA(NULL, pathA, CSIDL_APPDATA, FALSE));
    CHECK("SHGetSpecialFolderPathA returns TRUE", ok);
    HOOK_LIVE_A("SHGetSpecialFolderPathA", pathA);
    CHECK("SHGetSpecialFolderPath A/W agree", strcmp(pathA, modulePathA) == 0);
    showInfoA("SHGetSpecialFolderPathA");

    // --- SHGetSpecialFolderLocation --------------------------------------
    LPITEMIDLIST pidl = 0;
    CHECK_ABI("SHGetSpecialFolderLocation",
              hr = SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl));
    CHECK("SHGetSpecialFolderLocation S_OK", hr == S_OK);
    CHECK("SHGetSpecialFolderLocation pidl non-NULL", pidl != NULL);
    if (pidl) {
        SHGetPathFromIDListW(pidl, pathW);
        HOOK_LIVE_W("SHGetSpecialFolderLocation", pathW);
        showInfoW("SHGetSpecialFolderLocation");
        SHGetPathFromIDListA(pidl, pathA);
        showInfoA("SHGetSpecialFolderLocation");
        ILFree(pidl);
        pidl = 0;
    }

    // --- SHGetKnownFolderPath (CoTaskMemAlloc'd buffer) ------------------
    WCHAR *ppathW = NULL;
    CHECK_ABI("SHGetKnownFolderPath",
              hr = SHGetKnownFolderPath(&FOLDERID_RoamingAppData, 0, NULL, &ppathW));
    CHECK("SHGetKnownFolderPath S_OK", hr == S_OK);
    CHECK("SHGetKnownFolderPath buffer non-NULL", ppathW != NULL);
    if (ppathW) {
        HOOK_LIVE_W("SHGetKnownFolderPath", ppathW);
        CHECK("SHGetKnownFolderPath == module dir", wcscmp(ppathW, modulePathW) == 0);
    }
    showInfopW("SHGetKnownFolderPath", ppathW);

    // --- SHGetKnownFolderIDList ------------------------------------------
    CHECK_ABI("SHGetKnownFolderIDList",
              hr = SHGetKnownFolderIDList(&FOLDERID_RoamingAppData, 0, NULL, &pidl));
    CHECK("SHGetKnownFolderIDList S_OK", hr == S_OK);
    if (pidl) {
        SHGetPathFromIDListW(pidl, pathW);
        HOOK_LIVE_W("SHGetKnownFolderIDList", pathW);
        showInfoW("SHGetKnownFolderIDList");
        SHGetPathFromIDListA(pidl, pathA);
        showInfoA("SHGetKnownFolderIDList");
        ILFree(pidl);
        pidl = 0;
    }

    // --- SHGetFolderPathAndSubDir W / A (result is "<dir>\subN") ----------
    CHECK_ABI("SHGetFolderPathAndSubDirW",
              hr = SHGetFolderPathAndSubDirW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, L"sub1", pathW));
    // Returns S_OK on create, S_FALSE if the dir already exists (re-run); both succeed.
    CHECK("SHGetFolderPathAndSubDirW succeeded", SUCCEEDED(hr));
    HOOK_LIVE_W_PREFIX("SHGetFolderPathAndSubDirW", pathW);
    CHECK("SHGetFolderPathAndSubDirW appends \\sub1",
          wcsstr(pathW, L"\\sub1") != NULL);
    showInfoW("SHGetFolderPathAndSubDirW");

    CHECK_ABI("SHGetFolderPathAndSubDirA",
              hr = SHGetFolderPathAndSubDirA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, "sub2", pathA));
    CHECK("SHGetFolderPathAndSubDirA succeeded", SUCCEEDED(hr));
    HOOK_LIVE_A_PREFIX("SHGetFolderPathAndSubDirA", pathA);
    CHECK("SHGetFolderPathAndSubDirA appends \\sub2",
          strstr(pathA, "\\sub2") != NULL);
    showInfoA("SHGetFolderPathAndSubDirA");

    // --- SHGetFolderPath W / A -------------------------------------------
    CHECK_ABI("SHGetFolderPathW",
              hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathW));
    CHECK("SHGetFolderPathW S_OK", hr == S_OK);
    HOOK_LIVE_W("SHGetFolderPathW", pathW);
    CHECK("SHGetFolderPathW == module dir", wcscmp(pathW, modulePathW) == 0);
    showInfoW("SHGetFolderPathW");

    CHECK_ABI("SHGetFolderPathA",
              hr = SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathA));
    CHECK("SHGetFolderPathA S_OK", hr == S_OK);
    HOOK_LIVE_A("SHGetFolderPathA", pathA);
    CHECK("SHGetFolderPath A/W agree", strcmp(pathA, modulePathA) == 0);
    showInfoA("SHGetFolderPathA");

    // --- SHGetFolderLocation ---------------------------------------------
    CHECK_ABI("SHGetFolderLocation",
              hr = SHGetFolderLocation(NULL, CSIDL_APPDATA, NULL, 0, &pidl));
    CHECK("SHGetFolderLocation S_OK", hr == S_OK);
    if (pidl) {
        SHGetPathFromIDListW(pidl, pathW);
        HOOK_LIVE_W("SHGetFolderLocation", pathW);
        showInfoW("SHGetFolderLocation");
        SHGetPathFromIDListA(pidl, pathA);
        showInfoA("SHGetFolderLocation");
        ILFree(pidl);
        pidl = 0;
    }

    // --- SHGetFolderPathEx -----------------------------------------------
    CHECK_ABI("SHGetFolderPathEx",
              hr = SHGetFolderPathEx(&FOLDERID_RoamingAppData, 0, NULL, pathW, MAX_PATH));
    CHECK("SHGetFolderPathEx S_OK", hr == S_OK);
    HOOK_LIVE_W("SHGetFolderPathEx", pathW);
    CHECK("SHGetFolderPathEx == module dir", wcscmp(pathW, modulePathW) == 0);
    showInfoW("SHGetFolderPathEx");

    // --- Pass-through / negative cases -----------------------------------
    // A CSIDL that is NOT in isHookCsidl() must fall through to the real
    // shell32 via the MinHook trampoline (this is the only path that
    // exercises OriginalFunc()). CSIDL_WINDOWS resolves to the Windows dir,
    // which must NOT equal our module directory.
    CHECK_ABI("SHGetFolderPathW (pass-through)",
              hr = SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, SHGFP_TYPE_CURRENT, pathW));
    CHECK("pass-through SHGetFolderPathW S_OK", hr == S_OK);
    CHECK("pass-through NOT redirected to module dir",
          wcscmp(pathW, modulePathW) != 0);
    {
        // Sanity: it should look like a real Windows directory.
        WCHAR winDir[MAX_PATH] = {0};
        GetWindowsDirectoryW(winDir, MAX_PATH);
        CHECK("pass-through returns real Windows dir", wcscmp(pathW, winDir) == 0);
    }
    printf("pass-through (CSIDL_WINDOWS): %ls\n", pathW);
    ZeroMemory(pathW, sizeof(pathW));

    // Pass-through for the rfid path too: a known folder not in isHookRfid()
    // (Fonts) must not be redirected.
    ppathW = NULL;
    CHECK_ABI("SHGetKnownFolderPath (pass-through)",
              hr = SHGetKnownFolderPath(&FOLDERID_Fonts, 0, NULL, &ppathW));
    CHECK("pass-through SHGetKnownFolderPath S_OK", hr == S_OK);
    if (ppathW) {
        CHECK("pass-through rfid NOT redirected",
              wcscmp(ppathW, modulePathW) != 0);
        printf("pass-through (FOLDERID_Fonts): %ls\n", ppathW);
        CoTaskMemFree(ppathW);
    }

    // --- Verdict ---------------------------------------------------------
    // 10 W/A hooks redirect observably (the two _Location rfid variants and
    // the pass-through calls are excluded from the count by design).
    if (g_hooked == 0) {
        printf("FAILED: hijack shim installed no hooks (ABI probe ran against "
               "unhooked shell32 -- result is meaningless)\n");
        g_failures++;
    } else {
        printf("hook liveness: %d hooked call(s) observably redirected\n", g_hooked);
    }

    if (g_failures != 0) {
        printf("FAILED: %d check(s) failed\n", g_failures);
    } else {
        printf("PASSED: hooks live, paths correct, stack balanced, pass-through intact\n");
    }

    fflush(stdout);
    ExitProcess((UINT) g_failures);
    return g_failures;
}
