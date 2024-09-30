#pragma once
#include <windows.h>
#include <cctype>
#include <string>
#include <shlobj.h>
#include <stdio.h>
#include <filesystem>
#include <functional>
#include <set>
#include <ModuleList\ModuleList.hpp>
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)

struct shared {
    FARPROC DllCanUnloadNow;
    FARPROC DllGetClassObject;
    FARPROC DllRegisterServer;
    FARPROC DllUnregisterServer;
    FARPROC DebugSetMute;

    void LoadOriginalLibrary(HMODULE dll) {
        DllCanUnloadNow = GetProcAddress(dll, "DllCanUnloadNow");
        DllGetClassObject = GetProcAddress(dll, "DllGetClassObject");
        DllRegisterServer = GetProcAddress(dll, "DllRegisterServer");
        DllUnregisterServer = GetProcAddress(dll, "DllUnregisterServer");
        DebugSetMute = GetProcAddress(dll, "DebugSetMute");
    }
} shared;

struct dinput8_dll {
    HMODULE dll;
    FARPROC DirectInput8Create;

    void LoadOriginalLibrary(HMODULE module) {
        dll = module;
        shared.LoadOriginalLibrary(dll);
        DirectInput8Create = GetProcAddress(dll, "DirectInput8Create");
    }
} dinput8;

#pragma runtime_checks( "", off )

#ifdef _DEBUG
#pragma message ("You are compiling the code in Debug - be warned that wrappers for export functions may not have correct code generated")
#endif

typedef HRESULT(*fn_DirectInput8Create)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);
void _DirectInput8Create() { (fn_DirectInput8Create)dinput8.DirectInput8Create(); }

void _DllRegisterServer() { shared.DllRegisterServer(); }
void _DllUnregisterServer() { shared.DllUnregisterServer(); }
void _DllCanUnloadNow() { shared.DllCanUnloadNow(); }
void _DllGetClassObject() { shared.DllGetClassObject(); }
void _DebugSetMute() { shared.DebugSetMute(); }

#pragma runtime_checks( "", restore )
