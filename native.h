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
} HookTableSt;

typedef struct _ModulePathBuffer {
  size_t lengthA;
  size_t lengthW;
  char pathA[MAX_PATH];
  wchar_t pathW[MAX_PATH];
} ModulePathBuffer;

NTSYSAPI NTSTATUS RtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PCWCH UnicodeString,
    ULONG BytesInUnicodeString
);

NTSYSAPI NTSTATUS RtlMultiByteToUnicodeN(
    PWCH UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    const CHAR *MultiByteString,
    ULONG BytesInMultiByteString
);

static bool NtCreateDirectoryW(PCWSTR pszFileName) {
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

static bool NtCreateDirectoryA(PCSTR pszFileName) {
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
#endif //DLL_HIJACK_NATIVE_H
