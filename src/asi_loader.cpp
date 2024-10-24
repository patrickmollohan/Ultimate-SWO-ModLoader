#include "asi_loader.hpp"
#include "exception.hpp"
#include <initguid.h>
#include <filesystem>
#include <memory>
#include <FunctionHookMinHook.hpp>
#include <ModuleList\ModuleList.hpp>

#pragma runtime_checks( "", off )
#pragma intrinsic(_ReturnAddress)

struct shared {
    FARPROC DllCanUnloadNow;
    FARPROC DllGetClassObject;
    FARPROC DllRegisterServer;
    FARPROC DllUnregisterServer;

    void LoadOriginalLibrary(HMODULE dll) {
        DllCanUnloadNow = GetProcAddress(dll, "DllCanUnloadNow");
        DllGetClassObject = GetProcAddress(dll, "DllGetClassObject");
        DllRegisterServer = GetProcAddress(dll, "DllRegisterServer");
        DllUnregisterServer = GetProcAddress(dll, "DllUnregisterServer");
    }
} shared;

struct version_dll {
    HMODULE dll;
    FARPROC GetFileVersionInfoA;
    FARPROC GetFileVersionInfoByHandle;
    FARPROC GetFileVersionInfoExA;
    FARPROC GetFileVersionInfoExW;
    FARPROC GetFileVersionInfoSizeA;
    FARPROC GetFileVersionInfoSizeExA;
    FARPROC GetFileVersionInfoSizeExW;
    FARPROC GetFileVersionInfoSizeW;
    FARPROC GetFileVersionInfoW;
    FARPROC VerFindFileA;
    FARPROC VerFindFileW;
    FARPROC VerInstallFileA;
    FARPROC VerInstallFileW;
    FARPROC VerLanguageNameA;
    FARPROC VerLanguageNameW;
    FARPROC VerQueryValueA;
    FARPROC VerQueryValueW;

    void LoadOriginalLibrary(HMODULE module) {
        dll = module;
        shared.LoadOriginalLibrary(dll);
        GetFileVersionInfoA = GetProcAddress(dll, "GetFileVersionInfoA");
        GetFileVersionInfoByHandle = GetProcAddress(dll, "GetFileVersionInfoByHandle");
        GetFileVersionInfoExA = GetProcAddress(dll, "GetFileVersionInfoExA");
        GetFileVersionInfoExW = GetProcAddress(dll, "GetFileVersionInfoExW");
        GetFileVersionInfoSizeA = GetProcAddress(dll, "GetFileVersionInfoSizeA");
        GetFileVersionInfoSizeExA = GetProcAddress(dll, "GetFileVersionInfoSizeExA");
        GetFileVersionInfoSizeExW = GetProcAddress(dll, "GetFileVersionInfoSizeExW");
        GetFileVersionInfoSizeW = GetProcAddress(dll, "GetFileVersionInfoSizeW");
        GetFileVersionInfoW = GetProcAddress(dll, "GetFileVersionInfoW");
        VerFindFileA = GetProcAddress(dll, "VerFindFileA");
        VerFindFileW = GetProcAddress(dll, "VerFindFileW");
        VerInstallFileA = GetProcAddress(dll, "VerInstallFileA");
        VerInstallFileW = GetProcAddress(dll, "VerInstallFileW");
        VerLanguageNameA = GetProcAddress(dll, "VerLanguageNameA");
        VerLanguageNameW = GetProcAddress(dll, "VerLanguageNameW");
        VerQueryValueA = GetProcAddress(dll, "VerQueryValueA");
        VerQueryValueW = GetProcAddress(dll, "VerQueryValueW");
    }
} version;

void ASILoader::_GetFileVersionInfoA() { version.GetFileVersionInfoA(); }
void ASILoader::_GetFileVersionInfoByHandle() { version.GetFileVersionInfoByHandle(); }
void ASILoader::_GetFileVersionInfoExA() { version.GetFileVersionInfoExA(); }
void ASILoader::_GetFileVersionInfoExW() { version.GetFileVersionInfoExW(); }
void ASILoader::_GetFileVersionInfoSizeA() { version.GetFileVersionInfoSizeA(); }
void ASILoader::_GetFileVersionInfoSizeExA() { version.GetFileVersionInfoSizeExA(); }
void ASILoader::_GetFileVersionInfoSizeExW() { version.GetFileVersionInfoSizeExW(); }
void ASILoader::_GetFileVersionInfoSizeW() { version.GetFileVersionInfoSizeW(); }
void ASILoader::_GetFileVersionInfoW() { version.GetFileVersionInfoW(); }
void ASILoader::_VerFindFileA() { version.VerFindFileA(); }
void ASILoader::_VerFindFileW() { version.VerFindFileW(); }
void ASILoader::_VerInstallFileA() { version.VerInstallFileA(); }
void ASILoader::_VerInstallFileW() { version.VerInstallFileW(); }
void ASILoader::_VerLanguageNameA() { version.VerLanguageNameA(); }
void ASILoader::_VerLanguageNameW() { version.VerLanguageNameW(); }
void ASILoader::_VerQueryValueA() { version.VerQueryValueA(); }
void ASILoader::_VerQueryValueW() { version.VerQueryValueW(); }

void ASILoader::_DllRegisterServer() { shared.DllRegisterServer(); }
void ASILoader::_DllUnregisterServer() { shared.DllUnregisterServer(); }
void ASILoader::_DllCanUnloadNow() { shared.DllCanUnloadNow(); }
void ASILoader::_DllGetClassObject() { shared.DllGetClassObject(); }

bool WINAPI ASILoader::IsUltimateASILoader() { return true; }

void* ogMemModule = NULL;
void* WINAPI ASILoader::GetMemoryModule() { return ogMemModule; }

HMODULE hm;
std::vector<std::wstring> iniPaths;
std::filesystem::path gamePath;
std::wstring sLoadFromAPI;
thread_local std::string sCurrentFindFileDirA;
thread_local std::wstring sCurrentFindFileDirW;

bool iequals(std::wstring_view s1, std::wstring_view s2) {
    std::wstring str1(std::move(s1));
    std::wstring str2(std::move(s2));
    std::transform(str1.begin(), str1.end(), str1.begin(), [](wchar_t c) { return ::towlower(c); });
    std::transform(str2.begin(), str2.end(), str2.begin(), [](wchar_t c) { return ::towlower(c); });
    return (str1 == str2);
}

std::filesystem::path lexicallyRelativeCaseIns(const std::filesystem::path& path, const std::filesystem::path& base) {
    class input_iterator_range {
        public:
            input_iterator_range(const std::filesystem::path::const_iterator& first, const std::filesystem::path::const_iterator& last)
                : _first(first)
                , _last(last)
            {}
            std::filesystem::path::const_iterator begin() const { return _first; }
            std::filesystem::path::const_iterator end() const { return _last; }
        private:
            std::filesystem::path::const_iterator _first;
            std::filesystem::path::const_iterator _last;
    };

    if (!iequals(path.root_name().wstring(), base.root_name().wstring()) || path.is_absolute() != base.is_absolute() || (!path.has_root_directory() && base.has_root_directory())) {
        return std::filesystem::path();
    }
    std::filesystem::path::const_iterator a = path.begin(), b = base.begin();
    while (a != path.end() && b != base.end() && iequals(a->wstring(), b->wstring())) {
        ++a;
        ++b;
    }
    if (a == path.end() && b == base.end()) {
        return std::filesystem::path(".");
    }
    int count = 0;
    for (const auto& element : input_iterator_range(b, base.end())) {
        if (element != "." && element != "" && element != "..") {
            ++count;
        }
        else if (element == "..") {
            --count;
        }
    }
    if (count < 0) {
        return std::filesystem::path();
    }
    std::filesystem::path result;
    for (int i = 0; i < count; ++i) {
        result /= "..";
    }
    for (const auto& element : input_iterator_range(a, path.end())) {
        result /= element;
    }
    return result;
}

std::wstring to_wstring(std::string_view cstr) {
    std::string str(std::move(cstr));
    auto charsReturned = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(charsReturned, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], charsReturned);
    return wstrTo;
}

std::wstring SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD dwFlags, HANDLE hToken) {
    std::wstring r;
    WCHAR* szSystemPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(rfid, dwFlags, hToken, &szSystemPath))) {
        r = szSystemPath;
    }
    CoTaskMemFree(szSystemPath);
    return r;
};

std::wstring GetCurrentDirectoryW() {
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;
    std::wstring ret;
    auto bufferSize = INITIAL_BUFFER_SIZE;
    for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations) {
        ret.resize(bufferSize);
        auto charsReturned = GetCurrentDirectoryW(bufferSize, ret.data());
        if (charsReturned < ret.length()) {
            ret.resize(charsReturned);
            return ret;
        } else {
            bufferSize *= 2;
        }
    }
    return L"";
}

std::wstring GetModulePath(HMODULE hModule) {
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;
    std::wstring ret;
    auto bufferSize = INITIAL_BUFFER_SIZE;
    for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations) {
        ret.resize(bufferSize);
        size_t charsReturned = 0;
        charsReturned = GetModuleFileNameW(hModule, ret.data(), bufferSize);
        if (charsReturned < ret.length()) {
            ret.resize(charsReturned);
            return ret;
        } else {
            bufferSize *= 2;
        }
    }
    return std::wstring();
}

std::wstring GetExeModulePath() {
    std::wstring r = GetModulePath(NULL);
    r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

UINT GetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, const std::vector<std::wstring>& fileNames) {
    for (const auto& file : fileNames) {
        nDefault = GetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, file.c_str());
    }
    return nDefault;
}

std::wstring GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR szDefault, const std::vector<std::wstring>& fileNames) {
    std::wstring ret(szDefault);
    ret.resize(MAX_PATH);
    for (const auto& file : fileNames) {
        GetPrivateProfileStringW(lpAppName, lpKeyName, ret.data(), ret.data(), (DWORD)ret.size(), file.c_str());
    }
    return ret.data();
}

std::wstring GetSelfName() {
    const std::wstring moduleFileName = GetModuleFileNameW(hm);
    return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

template<typename T, typename... Args>
void GetSections(T&& h, Args... args) {
    const std::set< std::string_view, std::less<> > s = { args... };
    size_t dwLoadOffset = (size_t)GetModuleHandle(NULL);
    BYTE* pImageBase = reinterpret_cast<BYTE *>(dwLoadOffset);
    PIMAGE_DOS_HEADER   pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(dwLoadOffset);
    PIMAGE_NT_HEADERS   pNtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(pImageBase + pDosHeader->e_lfanew);
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHeader);
    for (int iSection = 0; iSection < pNtHeader->FileHeader.NumberOfSections; ++iSection, ++pSection) {
        auto pszSectionName = reinterpret_cast<const char*>(pSection->Name);
        if (s.find(pszSectionName) != s.end()) {
            DWORD dwPhysSize = (pSection->Misc.VirtualSize + 4095) & ~4095;
            std::forward<T>(h)(pSection, dwLoadOffset, dwPhysSize);
        }
    }
}

enum Kernel32ExportsNames {
    eGetStartupInfoA,
    eGetStartupInfoW,
    eGetModuleHandleA,
    eGetModuleHandleW,
    eGetProcAddress,
    eGetShortPathNameA,
    eFindFirstFileA,
    eFindNextFileA,
    eFindFirstFileW,
    eFindNextFileW,
    eFindFirstFileExA,
    eFindFirstFileExW,
    eLoadLibraryExA,
    eLoadLibraryExW,
    eLoadLibraryA,
    eLoadLibraryW,
    eFreeLibrary,
    eCreateEventA,
    eCreateEventW,
    eGetSystemInfo,
    eInterlockedCompareExchange,
    eSleep,
    eGetSystemTimeAsFileTime,
    eGetCurrentProcessId,
    eGetCommandLineA,
    eGetCommandLineW,
    eAcquireSRWLockExclusive,
    eCreateFileA,
    eCreateFileW,
    eGetFileAttributesA,
    eGetFileAttributesW,
    eGetFileAttributesExA,
    eGetFileAttributesExW,

    Kernel32ExportsNamesCount
};

enum Kernel32ExportsData {
    IATPtr,
    ProcAddress,

    Kernel32ExportsDataCount
};

enum OLE32ExportsNames {
    eCoCreateInstance,

    OLE32ExportsNamesCount
};

size_t Kernel32Data[Kernel32ExportsNamesCount][Kernel32ExportsDataCount];
size_t OLE32Data[OLE32ExportsNamesCount][Kernel32ExportsDataCount];

HMODULE LoadLib(const std::wstring& lpLibFileName);
static LONG OriginalLibraryLoaded = 0;

void LoadOriginalLibrary() {
    if (_InterlockedCompareExchange(&OriginalLibraryLoaded, 1, 0) != 0) return;

    auto szSelfName = GetSelfName();
    auto szSystemPath = SHGetKnownFolderPath(FOLDERID_System, 0, nullptr) + L'\\' + szSelfName;
    auto szLocalPath = GetModuleFileNameW(hm); 
    szLocalPath = szLocalPath.substr(0, szLocalPath.find_last_of(L"/\\") + 1);
    szLocalPath += L"versionHooked.dll";
    if (std::filesystem::exists(szLocalPath)) {
        version.LoadOriginalLibrary(LoadLib(szLocalPath));
    } else {
        version.LoadOriginalLibrary(LoadLib(szSystemPath));
    }
}

void FindFiles(WIN32_FIND_DATAW* fd) {
    auto dir = GetCurrentDirectoryW();

    HANDLE asiFile = FindFirstFileW(L"*.asi", fd);
    if (asiFile != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                auto pos = wcslen(fd->cFileName);

                if (fd->cFileName[pos - 4] == '.' &&
                        (fd->cFileName[pos - 3] == 'a' || fd->cFileName[pos - 3] == 'A') &&
                        (fd->cFileName[pos - 2] == 's' || fd->cFileName[pos - 2] == 'S') &&
                        (fd->cFileName[pos - 1] == 'i' || fd->cFileName[pos - 1] == 'I')) {
                    auto path = dir + L'\\' + fd->cFileName;

                    if (GetModuleHandle(path.c_str()) == NULL) {
                        auto h = LoadLib(path);
                        SetCurrentDirectoryW(dir.c_str()); //in case asi switched it

                        if (h == NULL) {
                            auto e = GetLastError();
                            if (e != ERROR_DLL_INIT_FAILED && e != ERROR_BAD_EXE_FORMAT) { // in case dllmain returns false or IMAGE_MACHINE is not compatible
                                std::wstring msg = L"Unable to load " + std::wstring(fd->cFileName) + L". Error: " + std::to_wstring(e) + L".";
                                if (e == ERROR_MOD_NOT_FOUND) {
                                    msg += L" This ASI file requires a dependency that is missing from your system. " \
                                        "To identify the missing dependency, download and run the free, open-source app, " \
                                        "Dependencies at https://github.com/lucasg/Dependencies";
                                }
                                MessageBoxW(0, msg.c_str(), L"ASI Loader", MB_ICONERROR);
                            }
                        } else {
                            auto procedure = (void(*)())GetProcAddress(h, "InitializeASI");
                            if (procedure != NULL) {
                                procedure();
                            }
                        }
                    }
                }
            }
        }
        while (FindNextFileW(asiFile, fd));
        FindClose(asiFile);
    }
}

void LoadPlugins() {
    auto oldDir = GetCurrentDirectoryW(); // store the current directory

    auto szSelfPath = GetModuleFileNameW(hm).substr(0, GetModuleFileNameW(hm).find_last_of(L"/\\") + 1);
    SetCurrentDirectoryW(szSelfPath.c_str());

    auto nWantsToLoadPlugins = GetPrivateProfileIntW(L"globalsets", L"loadplugins", TRUE, iniPaths);
    auto nWantsToLoadFromScriptsOnly = GetPrivateProfileIntW(L"globalsets", L"loadfromscriptsonly", FALSE, iniPaths);

    if (nWantsToLoadPlugins) {
        WIN32_FIND_DATAW fd;
        if (!nWantsToLoadFromScriptsOnly) FindFiles(&fd);

        SetCurrentDirectoryW(szSelfPath.c_str());

        if (SetCurrentDirectoryW(L"scripts\\")) FindFiles(&fd);

        SetCurrentDirectoryW(szSelfPath.c_str());

        if (SetCurrentDirectoryW(L"plugins\\")) FindFiles(&fd);

        SetCurrentDirectoryW(szSelfPath.c_str());

    }

    SetCurrentDirectoryW(oldDir.c_str()); // Reset the current directory
}

static LONG LoadedPluginsYet = 0;
void LoadEverything() {
    if (_InterlockedCompareExchange(&LoadedPluginsYet, 1, 0) != 0) return;

    LoadOriginalLibrary();

    LoadPlugins();
}

static LONG RestoredOnce = 0;
void LoadPluginsAndRestoreIAT(uintptr_t retaddr, std::wstring_view calledFrom = L"") {
    if (!sLoadFromAPI.empty() && calledFrom != sLoadFromAPI) return;

    bool calledFromBind = false;

    //steam drm check
    GetSections([&](PIMAGE_SECTION_HEADER pSection, size_t dwLoadOffset, DWORD dwPhysSize) {
        auto dwStart = static_cast<uintptr_t>(dwLoadOffset + pSection->VirtualAddress);
        auto dwEnd = dwStart + dwPhysSize;
        if (retaddr >= dwStart && retaddr <= dwEnd) calledFromBind = true;
    }, ".bind");

    if (calledFromBind) return;

    if (_InterlockedCompareExchange(&RestoredOnce, 1, 0) != 0) return;

    for (size_t i = 0; i < Kernel32ExportsNamesCount; i++) {
        if (Kernel32Data[i][IATPtr] && Kernel32Data[i][ProcAddress]) {
            auto ptr = (size_t*)Kernel32Data[i][IATPtr];
            DWORD dwProtect[2];
            VirtualProtect(ptr, sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
            *ptr = Kernel32Data[i][ProcAddress];
            VirtualProtect(ptr, sizeof(size_t), dwProtect[0], &dwProtect[1]);
        }
    }

    LoadEverything();
}

#define value_orA(path1, path2) (path1.empty() ? path2 : path1.string().c_str())
#define value_orW(path1, path2) (path1.empty() ? path2 : path1.wstring().c_str())

HMODULE LoadLib(const std::wstring& lpLibFileName) {
    return LoadLibraryW(lpLibFileName.c_str());
}

void WINAPI CustomGetStartupInfoA(LPSTARTUPINFOA lpStartupInfo) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetStartupInfoA");
    return GetStartupInfoA(lpStartupInfo);
}

void WINAPI CustomGetStartupInfoW(LPSTARTUPINFOW lpStartupInfo) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetStartupInfoW");
    return GetStartupInfoW(lpStartupInfo);
}

HMODULE WINAPI CustomGetModuleHandleA(LPCSTR lpModuleName) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetModuleHandleA");
    return GetModuleHandleA(lpModuleName);
}

HMODULE WINAPI CustomGetModuleHandleW(LPCWSTR lpModuleName) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetModuleHandleW");
    return GetModuleHandleW(lpModuleName);
}

FARPROC WINAPI CustomGetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetProcAddress");
    return GetProcAddress(hModule, lpProcName);
}

DWORD WINAPI CustomGetShortPathNameA(LPCSTR lpszLongPath, LPSTR lpszShortPath, DWORD cchBuffer) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetShortPathNameA");
    return GetShortPathNameA(lpszLongPath, lpszShortPath, cchBuffer);
}

BOOL WINAPI CustomFreeLibrary(HMODULE hLibModule) {
    if (hLibModule != hm) {
        return FreeLibrary(hLibModule);
    } else {
        return !NULL;
    }
}

HANDLE WINAPI CustomCreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"CreateEventA");
    return CreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
}

HANDLE WINAPI CustomCreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"CreateEventW");
    return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
}

void WINAPI CustomGetSystemInfo(LPSYSTEM_INFO lpSystemInfo) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetSystemInfo");
    return GetSystemInfo(lpSystemInfo);
}

LONG WINAPI CustomInterlockedCompareExchange(LONG volatile* Destination, LONG ExChange, LONG Comperand) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"InterlockedCompareExchange");
    return _InterlockedCompareExchange(Destination, ExChange, Comperand);
}

void WINAPI CustomSleep(DWORD dwMilliseconds) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"Sleep");
    return Sleep(dwMilliseconds);
}

void WINAPI CustomGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetSystemTimeAsFileTime");
    return GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

DWORD WINAPI CustomGetCurrentProcessId() {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetCurrentProcessId");
    return GetCurrentProcessId();
}

LPSTR WINAPI CustomGetCommandLineA() {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetCommandLineA");
    return GetCommandLineA();
}

LPWSTR WINAPI CustomGetCommandLineW() {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"GetCommandLineW");
    return GetCommandLineW();
}

void WINAPI CustomAcquireSRWLockExclusive(PSRWLOCK SRWLock) {
    LoadPluginsAndRestoreIAT((uintptr_t)_ReturnAddress(), L"AcquireSRWLockExclusive");
    return AcquireSRWLockExclusive(SRWLock);
}

HRESULT WINAPI CustomCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv) {
    HRESULT hr = REGDB_E_KEYMISSING;
    HMODULE hDll = NULL;

    hDll = ::LoadLibrary(L"version.dll");

    if (hDll == NULL || GetProcAddress(hDll, "IsUltimateASILoader") != NULL)
        return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    typedef HRESULT(__stdcall *pDllGetClassObject)(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR* ppv);

    pDllGetClassObject GetClassObject = (pDllGetClassObject)::GetProcAddress(hDll, "DllGetClassObject");
    if (GetClassObject == NULL) {
        ::FreeLibrary(hDll);
        return hr;
    }

    IClassFactory *pIFactory;

    hr = GetClassObject(rclsid, IID_IClassFactory, (LPVOID *)&pIFactory);

    if (!SUCCEEDED(hr)) return hr;

    hr = pIFactory->CreateInstance(pUnkOuter, riid, ppv);
    pIFactory->Release();

    return hr;
}

std::vector<std::string> importedModulesList;
bool HookKernel32IAT(HMODULE mod, bool exe) {
    auto hExecutableInstance = (size_t)mod;
    IMAGE_NT_HEADERS*           ntHeader = (IMAGE_NT_HEADERS*)(hExecutableInstance + ((IMAGE_DOS_HEADER*)hExecutableInstance)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR*    pImports = (IMAGE_IMPORT_DESCRIPTOR*)(hExecutableInstance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    size_t                      nNumImports = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size / sizeof(IMAGE_IMPORT_DESCRIPTOR) - 1;

    if (exe) {
        Kernel32Data[eGetStartupInfoA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetStartupInfoA");
        Kernel32Data[eGetStartupInfoW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetStartupInfoW");
        Kernel32Data[eGetModuleHandleA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetModuleHandleA");
        Kernel32Data[eGetModuleHandleW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetModuleHandleW");
        Kernel32Data[eGetProcAddress][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetProcAddress");
        Kernel32Data[eGetShortPathNameA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetShortPathNameA");
        Kernel32Data[eFindFirstFileA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FindFirstFileA");
        Kernel32Data[eFindNextFileA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FindNextFileA");
        Kernel32Data[eFindFirstFileW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FindFirstFileW");
        Kernel32Data[eFindNextFileW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FindNextFileW");
        Kernel32Data[eFindFirstFileExA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FindFirstFileExA");
        Kernel32Data[eFindFirstFileExW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FindFirstFileExW");
        Kernel32Data[eLoadLibraryExA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "LoadLibraryExA");
        Kernel32Data[eLoadLibraryExW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "LoadLibraryExW");
        Kernel32Data[eLoadLibraryA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "LoadLibraryA");
        Kernel32Data[eLoadLibraryW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "LoadLibraryW");
        Kernel32Data[eFreeLibrary][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "FreeLibrary");
        Kernel32Data[eCreateEventA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "CreateEventA");
        Kernel32Data[eCreateEventW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "CreateEventW");
        Kernel32Data[eGetSystemInfo][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetSystemInfo");
        Kernel32Data[eInterlockedCompareExchange][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "InterlockedCompareExchange");
        Kernel32Data[eSleep][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "Sleep");
        Kernel32Data[eGetSystemTimeAsFileTime][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetSystemTimeAsFileTime");
        Kernel32Data[eGetCurrentProcessId][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetCurrentProcessId");
        Kernel32Data[eGetCommandLineA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetCommandLineA");
        Kernel32Data[eGetCommandLineW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetCommandLineW");
        Kernel32Data[eAcquireSRWLockExclusive][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "AcquireSRWLockExclusive");
        Kernel32Data[eCreateFileA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "CreateFileA");
        Kernel32Data[eCreateFileW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "CreateFileW");
        Kernel32Data[eGetFileAttributesA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesA");
        Kernel32Data[eGetFileAttributesW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesW");
        Kernel32Data[eGetFileAttributesExA][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesExA");
        Kernel32Data[eGetFileAttributesExW][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("KERNEL32.DLL")), "GetFileAttributesExW");
    }

    uint32_t matchedImports = 0;

    auto PatchIAT = [&](size_t start, size_t end, size_t exe_end) {
        for (size_t i = 0; i < nNumImports; i++) {
            if (hExecutableInstance + (pImports + i)->FirstThunk > start && !(end && hExecutableInstance + (pImports + i)->FirstThunk > end)) end = hExecutableInstance + (pImports + i)->FirstThunk;
        }

        if (!end) { end = start + 0x100; }
        if (end > exe_end) { // for very broken exes
            start = hExecutableInstance;
            end = exe_end;
        }

        for (auto i = start; i < end; i += sizeof(size_t)) {
            DWORD dwProtect[2];
            VirtualProtect((size_t*)i, sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwProtect[0]);

            auto ptr = *(size_t*)i;
            if (!ptr) continue;

            if (ptr == Kernel32Data[eGetStartupInfoA][ProcAddress]) {
                if (exe) Kernel32Data[eGetStartupInfoA][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetStartupInfoA;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetStartupInfoW][ProcAddress]) {
                if (exe) Kernel32Data[eGetStartupInfoW][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetStartupInfoW;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetModuleHandleA][ProcAddress]) {
                if (exe) Kernel32Data[eGetModuleHandleA][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetModuleHandleA;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetModuleHandleW][ProcAddress]) {
                if (exe) Kernel32Data[eGetModuleHandleW][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetModuleHandleW;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetProcAddress][ProcAddress]) {
                if (exe) Kernel32Data[eGetProcAddress][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetProcAddress;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetShortPathNameA][ProcAddress]) {
                if (exe) Kernel32Data[eGetShortPathNameA][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetShortPathNameA;
                matchedImports++;
            } else if (ptr == Kernel32Data[eFreeLibrary][ProcAddress]) {
                if (exe) Kernel32Data[eFreeLibrary][IATPtr] = i;
                *(size_t*)i = (size_t)CustomFreeLibrary;
                matchedImports++;
            } else if (ptr == Kernel32Data[eCreateEventA][ProcAddress]) {
                if (exe) Kernel32Data[eCreateEventA][IATPtr] = i;
                *(size_t*)i = (size_t)CustomCreateEventA;
                matchedImports++;
            } else if (ptr == Kernel32Data[eCreateEventW][ProcAddress]) {
                if (exe) Kernel32Data[eCreateEventW][IATPtr] = i;
                *(size_t*)i = (size_t)CustomCreateEventW;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetSystemInfo][ProcAddress]) {
                if (exe) Kernel32Data[eGetSystemInfo][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetSystemInfo;
                matchedImports++;
            } else if (ptr == Kernel32Data[eInterlockedCompareExchange][ProcAddress]) {
                if (exe) Kernel32Data[eInterlockedCompareExchange][IATPtr] = i;
                *(size_t*)i = (size_t)CustomInterlockedCompareExchange;
                matchedImports++;
            } else if (ptr == Kernel32Data[eSleep][ProcAddress]) {
                if (exe) Kernel32Data[eSleep][IATPtr] = i;
                *(size_t*)i = (size_t)CustomSleep;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetSystemTimeAsFileTime][ProcAddress]) {
                if (exe) Kernel32Data[eGetSystemTimeAsFileTime][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetSystemTimeAsFileTime;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetCurrentProcessId][ProcAddress]) {
                if (exe) Kernel32Data[eGetCurrentProcessId][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetCurrentProcessId;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetCommandLineA][ProcAddress]) {
                if (exe) Kernel32Data[eGetCommandLineA][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetCommandLineA;
                matchedImports++;
            } else if (ptr == Kernel32Data[eGetCommandLineW][ProcAddress]) {
                if (exe) Kernel32Data[eGetCommandLineW][IATPtr] = i;
                *(size_t*)i = (size_t)CustomGetCommandLineW;
                matchedImports++;
            } else if (ptr == Kernel32Data[eAcquireSRWLockExclusive][ProcAddress]) {
                if (exe) Kernel32Data[eAcquireSRWLockExclusive][IATPtr] = i;
                *(size_t*)i = (size_t)CustomAcquireSRWLockExclusive;
                matchedImports++;
            }
            VirtualProtect((size_t*)i, sizeof(size_t), dwProtect[0], &dwProtect[1]);
        }
    };

    auto PatchCoCreateInstance = [&](size_t start, size_t end, size_t exe_end) {
        if (iequals(GetSelfName(), L"version.dll")) return;

        for (size_t i = 0; i < nNumImports; i++) {
            if (hExecutableInstance + (pImports + i)->FirstThunk > start && !(end && hExecutableInstance + (pImports + i)->FirstThunk > end)) end = hExecutableInstance + (pImports + i)->FirstThunk;
        }

        if (!end) { end = start + 0x100; }
        if (end > exe_end) { // for very broken exes
            start = hExecutableInstance;
            end = exe_end;
        }

        if (exe) OLE32Data[eCoCreateInstance][ProcAddress] = (size_t)GetProcAddress(GetModuleHandle(TEXT("OLE32.DLL")), "CoCreateInstance");

        for (auto i = start; i < end; i += sizeof(size_t)) {
            DWORD dwProtect[2];
            VirtualProtect((size_t*)i, sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwProtect[0]);

            auto ptr = *(size_t*)i;
            if (!ptr) continue;

            if (ptr == OLE32Data[eCoCreateInstance][ProcAddress]) {
                if (exe) OLE32Data[eCoCreateInstance][IATPtr] = i;
                *(size_t*)i = (size_t)CustomCoCreateInstance;
                VirtualProtect((size_t*)i, sizeof(size_t), dwProtect[0], &dwProtect[1]);
                break;
            }

            VirtualProtect((size_t*)i, sizeof(size_t), dwProtect[0], &dwProtect[1]);
        }
    };

    static auto getSection = [](const PIMAGE_NT_HEADERS nt_headers, unsigned section) -> PIMAGE_SECTION_HEADER {
        return reinterpret_cast<PIMAGE_SECTION_HEADER>(
            (UCHAR*)nt_headers->OptionalHeader.DataDirectory +
            nt_headers->OptionalHeader.NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY) +
            section * sizeof(IMAGE_SECTION_HEADER));
    };

    static auto getSectionEnd = [](IMAGE_NT_HEADERS* ntHeader, size_t inst) -> auto {
        auto sec = getSection(ntHeader, ntHeader->FileHeader.NumberOfSections - 1);
        // .bind section may have vanished from the executable (test case: Yakuza 4)
        // so back to the first valid section if that happened
        while (sec->Misc.VirtualSize == 0) sec--;

        auto secSize = max(sec->SizeOfRawData, sec->Misc.VirtualSize);
        auto end = inst + max(sec->PointerToRawData, sec->VirtualAddress) + secSize;
        return end;
    };

    auto hExecutableInstance_end = getSectionEnd(ntHeader, hExecutableInstance);

    // Find kernel32.dll
    for (size_t i = 0; i < nNumImports; i++) {
        if ((size_t)(hExecutableInstance + (pImports + i)->Name) < hExecutableInstance_end) {
            if (!_stricmp((const char*)(hExecutableInstance + (pImports + i)->Name), "KERNEL32.DLL")) {
                PatchIAT(hExecutableInstance + (pImports + i)->FirstThunk, 0, hExecutableInstance_end);
            } else if (!_stricmp((const char*)(hExecutableInstance + (pImports + i)->Name), "OLE32.DLL")) {
                PatchCoCreateInstance(hExecutableInstance + (pImports + i)->FirstThunk, 0, hExecutableInstance_end);
            }
            importedModulesList.emplace_back((const char*)(hExecutableInstance + (pImports + i)->Name));
        }
    }

    return matchedImports > 0;
}

LONG WINAPI CustomUnhandledExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo) {
    // step 1: write minidump
    wchar_t     modulename[MAX_PATH];
    wchar_t     filename[MAX_PATH];
    wchar_t     timestamp[128];
    __time64_t  time;
    struct tm   ltime;
    HANDLE      hFile;
    HWND        hWnd;

    wchar_t* modulenameptr = nullptr;
    if (GetModuleFileNameW(GetModuleHandle(NULL), modulename, _countof(modulename)) != 0) {
        modulenameptr = wcsrchr(modulename, '\\');
        *modulenameptr = L'\0';
        modulenameptr += 1;
    } else {
        wcscpy_s(modulename, L"err.err");
    }

    _time64(&time);
    _localtime64_s(&ltime, &time);
    wcsftime(timestamp, _countof(timestamp), L"%Y%m%d%H%M%S", &ltime);
    swprintf_s(filename, L"%s\\%s\\%s.%s.dmp", modulename, L"CrashDumps", modulenameptr, timestamp);

    hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION ex;
        memset(&ex, 0, sizeof(ex));
        ex.ThreadId = GetCurrentThreadId();
        ex.ExceptionPointers = ExceptionInfo;
        ex.ClientPointers = TRUE;

        if (FAILED(MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithDataSegs, &ex, NULL, NULL))) {
        }

        CloseHandle(hFile);
    }

    // step 2: write log
    // Logs exception into buffer and writes to file
    swprintf_s(filename, L"%s\\%s\\%s.%s.log", modulename, L"CrashDumps", modulenameptr, timestamp);
    hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        auto Log = [ExceptionInfo, hFile](char* buffer, size_t size, bool reg, bool stack, bool trace) {
            if (LogException(buffer, size, (LPEXCEPTION_POINTERS)ExceptionInfo, reg, stack, trace)) {
                DWORD NumberOfBytesWritten = 0;
                WriteFile(hFile, buffer, (DWORD)strlen(buffer), &NumberOfBytesWritten, NULL);
            }
        };

        // Try to make a very descriptive exception, for that we need to malloc a huge buffer...
        if (auto buffer = (char*)malloc(max_logsize_ever)) {
            Log(buffer, max_logsize_ever, true, true, true);
            free(buffer);
        } else {
            // Use a static buffer, no need for any allocation
            static const auto size = max_logsize_basic + max_logsize_regs + max_logsize_stackdump;
            static char static_buf[size];
            static_assert(size <= max_static_buffer, "Static buffer is too big");

            Log(buffer = static_buf, sizeof(static_buf), true, true, false);
        }

        CloseHandle(hFile);
    }

    // step 3: exit the application
    ShowCursor(TRUE);
    hWnd = FindWindowW(0, L"");
    SetForegroundWindow(hWnd);

    return EXCEPTION_CONTINUE_SEARCH;
}

void Init() {
    std::wstring modulePath = GetModuleFileNameW(hm);
    std::wstring moduleName = modulePath.substr(modulePath.find_last_of(L"/\\") + 1);
    moduleName.resize(moduleName.find_last_of(L'.'));
    modulePath.resize(modulePath.find_last_of(L"/\\") + 1);
    iniPaths.emplace_back(modulePath + moduleName + L".ini");
    iniPaths.emplace_back(modulePath + L"global.ini");
    iniPaths.emplace_back(modulePath + L"scripts\\global.ini");
    iniPaths.emplace_back(modulePath + L"plugins\\global.ini");
    iniPaths.emplace_back(modulePath + L"update\\global.ini");

    auto nForceEPHook = GetPrivateProfileIntW(TEXT("globalsets"), TEXT("forceentrypointhook"), FALSE, iniPaths);
    auto nDontLoadFromDllMain = GetPrivateProfileIntW(TEXT("globalsets"), TEXT("dontloadfromdllmain"), TRUE, iniPaths);
    sLoadFromAPI = GetPrivateProfileStringW(TEXT("globalsets"), TEXT("loadfromapi"), L"", iniPaths);
    auto nFindModule = GetPrivateProfileIntW(TEXT("globalsets"), TEXT("findmodule"), FALSE, iniPaths);
    auto nDisableCrashDumps = GetPrivateProfileIntW(TEXT("globalsets"), TEXT("disablecrashdumps"), FALSE, iniPaths);

    auto FolderExists = [](auto szPath) -> bool {
        try {
            auto path = std::filesystem::path(szPath);
            if (path.is_absolute()) {
                return std::filesystem::is_directory(path);
            } else {
                return std::filesystem::is_directory(std::filesystem::path(GetModuleFileNameW(NULL)).parent_path() / path);
            }
        } catch (...) {}
        
        return false;
    };

    if (!nDisableCrashDumps) {
        if (FolderExists(L"CrashDumps")) {
            SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
            // Now stub out CustomUnhandledExceptionFilter so NO ONE ELSE can set it!
            uint32_t ret = 0x909090C3; //ret
            DWORD protect[2];
            VirtualProtect(&SetUnhandledExceptionFilter, sizeof(ret), PAGE_EXECUTE_READWRITE, &protect[0]);
            memcpy(&SetUnhandledExceptionFilter, &ret, sizeof(ret));
            VirtualProtect(&SetUnhandledExceptionFilter, sizeof(ret), protect[0], &protect[1]);
        }
    }

    if (nForceEPHook != FALSE || nDontLoadFromDllMain != FALSE) {
        HMODULE mainModule = GetModuleHandle(NULL);
        bool hookedSuccessfully = HookKernel32IAT(mainModule, true);
        if (!hookedSuccessfully) {
            LoadOriginalLibrary();
        }

        const auto it = std::find_if(std::begin(importedModulesList), std::end(importedModulesList), [&](const auto& str) { return iequals(L"unityplayer.dll", to_wstring(str)); } );
        const auto bUnityPlayerImported = it != std::end(importedModulesList);

        HMODULE m = mainModule;
        if (nFindModule || importedModulesList.size() <= 2 || bUnityPlayerImported) {
            ModuleList dlls;
            dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);

            auto ual = std::find_if(dlls.m_moduleList.begin(), dlls.m_moduleList.end(), [](auto const& it) {
                return std::get<HMODULE>(it) == hm;
            });

            auto sim = std::find_if(dlls.m_moduleList.rbegin(), dlls.m_moduleList.rend(), [&ual](auto const& it) {
                auto str1 = std::get<std::wstring>(*ual);
                auto str2 = std::get<std::wstring>(it);
                std::transform(str1.begin(), str1.end(), str1.begin(), [](wchar_t c) { return ::towlower(c); });
                std::transform(str2.begin(), str2.end(), str2.begin(), [](wchar_t c) { return ::towlower(c); });

                if (str2 == L"unityplayer")
                    return true;

                return (str2 != str1) && (str2.find(str1) != std::wstring::npos);
            });

            if (ual != dlls.m_moduleList.begin()) {
                if (sim != dlls.m_moduleList.rend()) {
                    m = std::get<HMODULE>(*sim);
                } else {
                    m = std::get<HMODULE>(*std::prev(ual, 1));
                }
            }
        }

        if (m != mainModule) {
            HookKernel32IAT(m, false);
        }
    } else {
        LoadEverything();
    }
}

bool ASILoader::Enable(HMODULE hModule) {
    hm = hModule;
    Init();
    return true;
}

void ASILoader::Disable() {
    for (size_t i = 0; i < OLE32ExportsNamesCount; i++) {
        if (OLE32Data[i][IATPtr] && OLE32Data[i][ProcAddress]) {
            auto ptr = (size_t*)OLE32Data[i][IATPtr];
            DWORD dwProtect[2];
            VirtualProtect(ptr, sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
            *ptr = OLE32Data[i][ProcAddress];
            VirtualProtect(ptr, sizeof(size_t), dwProtect[0], &dwProtect[1]);
        }
    }
}

#pragma runtime_checks( "", restore )
