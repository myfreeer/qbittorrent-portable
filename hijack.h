#pragma once
#ifndef __DLL_HIJACK_MAIN_HEADER__
#define __DLL_HIJACK_MAIN_HEADER__

#ifndef UNICODE
#define UNICODE
#endif // !UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // !_UNICODE

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN64
typedef uint64_t MWORD;
#else //_WIN64
typedef uint32_t MWORD;
#endif //_WIN64

#ifdef __cplusplus
#define EXTERNC extern "C"
#else //__cplusplus
#define EXTERNC
#endif //__cplusplus

#define EXPORT EXTERNC __declspec(dllexport) void __cdecl
#define PLACEHOLDER EXTERNC void __cdecl

#ifndef __GNUC__
#define DllMainCRTStartup DllMain
#define UNUSED
#else // __GNUC__
#define UNUSED __attribute__((unused))
#define __nop() asm ("nop")
#define __inbyte(seq) asm ("nop")
#endif // __GNUC__

extern void DLLHijackAttach(bool isSucceed);

extern void DLLHijackDetach(bool);

HINSTANCE hInstance = NULL;

#define NOP_FUNC(seq) { \
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __nop();\
    __inbyte(seq);\
}
//用__inbyte来生成一点不一样的代码，避免被VS自动合并相同函数

bool WriteMemory(PBYTE BaseAddress, PBYTE Buffer, DWORD nSize) {
    DWORD ProtectFlag = 0;
    if (VirtualProtectEx(GetCurrentProcess(), BaseAddress, nSize, PAGE_EXECUTE_READWRITE, &ProtectFlag)) {
        memcpy(BaseAddress, Buffer, nSize);
        FlushInstructionCache(GetCurrentProcess(), BaseAddress, nSize);
        VirtualProtectEx(GetCurrentProcess(), BaseAddress, nSize, ProtectFlag, &ProtectFlag);
        return true;
    }
    return false;
}

// 还原导出函数
void InstallJMP(PBYTE BaseAddress, MWORD Function) {
#ifdef _WIN64
    BYTE move[] = {0x48, 0xB8};//move rax,xxL);
    BYTE jump[] = {0xFF, 0xE0};//jmp rax

    WriteMemory(BaseAddress, move, sizeof(move));
    BaseAddress += sizeof(move);

    WriteMemory(BaseAddress, (PBYTE) &Function, sizeof(MWORD));
    BaseAddress += sizeof(MWORD);

    WriteMemory(BaseAddress, jump, sizeof(jump));
#else // _WIN64
    BYTE jump[] = {0xE9};
    WriteMemory(BaseAddress, jump, sizeof(jump));
    BaseAddress += sizeof(jump);

    MWORD offset = Function - (MWORD)BaseAddress - 4;
    WriteMemory(BaseAddress, (PBYTE)&offset, sizeof(offset));
#endif // _WIN64
}

// 加载系统dll
bool LoadSysDll(HINSTANCE hModule) {
    PBYTE pImageBase = (PBYTE) hModule;
    PIMAGE_DOS_HEADER pimDH = (PIMAGE_DOS_HEADER) pImageBase;
    if (pimDH->e_magic == IMAGE_DOS_SIGNATURE) {
        PIMAGE_NT_HEADERS pimNH = (PIMAGE_NT_HEADERS) (pImageBase + pimDH->e_lfanew);
        if (pimNH->Signature == IMAGE_NT_SIGNATURE) {
            PIMAGE_EXPORT_DIRECTORY pimExD = (PIMAGE_EXPORT_DIRECTORY)
                    (pImageBase + pimNH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
            DWORD *pName = (DWORD *) (pImageBase + pimExD->AddressOfNames);
            DWORD *pFunction = (DWORD *) (pImageBase + pimExD->AddressOfFunctions);
#ifndef DLL_HIJACK_NON_SYSTEM
            wchar_t szSysDirectory[MAX_PATH + 1];
            GetSystemDirectory(szSysDirectory, MAX_PATH);

            wchar_t szDllPath[MAX_PATH + 1];
            lstrcpy(szDllPath, szSysDirectory);
            lstrcat(szDllPath, TEXT("\\" DLL_NAME ".dll"));
#else
            wchar_t *szDllPath = TEXT(DLL_NAME ".dll");
#endif
            HINSTANCE hDllModule = LoadLibrary(szDllPath);
            hInstance = hDllModule;
            for (size_t i = 0; i < pimExD->NumberOfNames; i++) {
                MWORD Original = (MWORD) GetProcAddress(hDllModule, (char *) (pImageBase + pName[i]));
                if (Original) {
                    InstallJMP(pImageBase + pFunction[i], Original);
                }
            }
            return true;
        }
    }
    return false;
}

EXTERNC BOOL WINAPI DllMainCRTStartup(HINSTANCE hModule, DWORD dwReason, LPVOID UNUSED pv) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        // 保持系统dll原有功能
        DLLHijackAttach(LoadSysDll(hModule));
    } else if (dwReason == DLL_PROCESS_DETACH) {
        if (hInstance != NULL) {
            DLLHijackDetach((bool) FreeLibrary(hInstance));
        } else {
            DLLHijackDetach(false);
        }
    }
    return TRUE;
}

#endif // __DLL_HIJACK_MAIN_HEADER__
