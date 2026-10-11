#include <mutex>
#include <stdexcept>
#include <windows.h>
void initialize_loader();
namespace {
HMODULE real_version;
std::once_flag once;
FARPROC resolve(const char *name) {
  std::call_once(once, [] {
    wchar_t system[MAX_PATH];
    GetSystemDirectoryW(system, MAX_PATH);
    std::wstring path(system);
    path += L"\\version.dll";
    real_version = LoadLibraryW(path.c_str());
    if (!real_version)
      ExitProcess(126);
    initialize_loader();
  });
  auto address = GetProcAddress(real_version, name);
  if (!address)
    ExitProcess(127);
  return address;
}
} // namespace
#define FORWARD(result, name, signature, arguments)                            \
  extern "C" result WINAPI proxy_##name signature {                            \
    using Function = result(WINAPI *) signature;                               \
    return reinterpret_cast<Function>(resolve(#name)) arguments;               \
  }
FORWARD(BOOL, GetFileVersionInfoA, (LPCSTR a, DWORD b, DWORD c, LPVOID d),
        (a, b, c, d))
FORWARD(BOOL, GetFileVersionInfoW, (LPCWSTR a, DWORD b, DWORD c, LPVOID d),
        (a, b, c, d))
FORWARD(DWORD, GetFileVersionInfoSizeA, (LPCSTR a, LPDWORD b), (a, b))
FORWARD(DWORD, GetFileVersionInfoSizeW, (LPCWSTR a, LPDWORD b), (a, b))
FORWARD(BOOL, GetFileVersionInfoExA,
        (DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e), (a, b, c, d, e))
FORWARD(BOOL, GetFileVersionInfoExW,
        (DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e), (a, b, c, d, e))
FORWARD(DWORD, GetFileVersionInfoSizeExA, (DWORD a, LPCSTR b, LPDWORD c),
        (a, b, c))
FORWARD(DWORD, GetFileVersionInfoSizeExW, (DWORD a, LPCWSTR b, LPDWORD c),
        (a, b, c))
FORWARD(BOOL, VerQueryValueA, (LPCVOID a, LPCSTR b, LPVOID *c, PUINT d),
        (a, b, c, d))
FORWARD(BOOL, VerQueryValueW, (LPCVOID a, LPCWSTR b, LPVOID *c, PUINT d),
        (a, b, c, d))
FORWARD(DWORD, VerLanguageNameA, (DWORD a, LPSTR b, DWORD c), (a, b, c))
FORWARD(DWORD, VerLanguageNameW, (DWORD a, LPWSTR b, DWORD c), (a, b, c))
FORWARD(DWORD, VerFindFileA,
        (DWORD a, LPSTR b, LPSTR c, LPSTR d, LPSTR e, PUINT f, LPSTR g,
         PUINT h),
        (a, b, c, d, e, f, g, h))
FORWARD(DWORD, VerFindFileW,
        (DWORD a, LPWSTR b, LPWSTR c, LPWSTR d, LPWSTR e, PUINT f, LPWSTR g,
         PUINT h),
        (a, b, c, d, e, f, g, h))
FORWARD(DWORD, VerInstallFileA,
        (DWORD a, LPSTR b, LPSTR c, LPSTR d, LPSTR e, LPSTR f, LPSTR g,
         PUINT h),
        (a, b, c, d, e, f, g, h))
FORWARD(DWORD, VerInstallFileW,
        (DWORD a, LPWSTR b, LPWSTR c, LPWSTR d, LPWSTR e, LPWSTR f, LPWSTR g,
         PUINT h),
        (a, b, c, d, e, f, g, h))
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(instance);
  return TRUE;
}
