#ifndef DLL_HIJACK_NATIVE_H
#define DLL_HIJACK_NATIVE_H

#include <stdbool.h>
#include <winternl.h>

typedef LPVOID (WINAPI *pCoTaskMemAlloc)(SIZE_T cb);

typedef HRESULT (WINAPI *pSHParseDisplayName)(
    PCWSTR pszName,
    IBindCtx *pbc,
    PIDLIST_ABSOLUTE *ppidl,
    SFGAOF sfgaoIn,
    SFGAOF *psfgaoOut);

typedef struct _HookTable {
  void *original;
  void *hook;
  char *name;
} HookTable;

typedef struct _ModulePathBuffer {
  size_t lengthA;
  size_t lengthW;
  char pathA[MAX_PATH];
  wchar_t pathW[MAX_PATH];
} ModulePathBuffer;

static bool NtCreateDirectoryW(PCWSTR pszFileName) {
  NTSTATUS Status;
  UNICODE_STRING FileName;
  HANDLE DirectoryHandle;
  IO_STATUS_BLOCK IoStatus;
  OBJECT_ATTRIBUTES ObjectAttributes;

  if (!RtlDosPathNameToNtPathName_U(pszFileName, &FileName, NULL, NULL)) {
    return false;
  }

  InitializeObjectAttributes(&ObjectAttributes, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

  Status = NtCreateFile(&DirectoryHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL,
                        0);

  RtlFreeUnicodeString(&FileName);

  if (NT_SUCCESS(Status)) {
    NtClose(DirectoryHandle);
    return true;
  }
  return false;
}

static inline bool NtCreateDirectoryA(PCSTR pszFileName) {
  WCHAR buffer[MAX_PATH];
  UNICODE_STRING pathW = {0, MAX_PATH * sizeof(wchar_t), buffer};
  ANSI_STRING pathA;
  RtlInitAnsiString(&pathA, pszFileName);
  if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&pathW, &pathA, FALSE))) {
    return false;
  }
  return NtCreateDirectoryW(buffer);
}

static void *LdrGetDllFunction(const wchar_t *dll, const char *fn) {
  UNICODE_STRING dllName;
  RtlInitUnicodeString(&dllName, dll);

  PVOID handle;
  NTSTATUS status = LdrGetDllHandle(NULL, NULL, &dllName, &handle);

  if (!NT_SUCCESS(status))
    status = LdrLoadDll(NULL, NULL, &dllName, &handle);

  if (!NT_SUCCESS(status))
    return NULL;

  ANSI_STRING fnName;
  RtlInitAnsiString(&fnName, fn);

  PVOID proc;
  status = LdrGetProcedureAddress(handle, &fnName, 0, &proc);

  return NT_SUCCESS(status) ? proc : NULL;
}
#endif //DLL_HIJACK_NATIVE_H
