#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

int wmain(int argc, wchar_t **argv) {
  if (argc != 2)
    return 2;
  SetEnvironmentVariableW(L"ANNO5_MODS_DISABLED", L"1");
  auto proxy = LoadLibraryW(argv[1]);
  wchar_t system[MAX_PATH];
  if (!proxy || !GetSystemDirectoryW(system, MAX_PATH))
    return 3;
  auto real = LoadLibraryW((std::wstring(system) + L"\\version.dll").c_str());
  if (!real)
    return 4;
  const char *exports[] = {"GetFileVersionInfoA",
                           "GetFileVersionInfoW",
                           "GetFileVersionInfoSizeA",
                           "GetFileVersionInfoSizeW",
                           "GetFileVersionInfoExA",
                           "GetFileVersionInfoExW",
                           "GetFileVersionInfoSizeExA",
                           "GetFileVersionInfoSizeExW",
                           "VerQueryValueA",
                           "VerQueryValueW",
                           "VerLanguageNameA",
                           "VerLanguageNameW",
                           "VerFindFileA",
                           "VerFindFileW",
                           "VerInstallFileA",
                           "VerInstallFileW"};
  for (auto name : exports)
    if (!GetProcAddress(proxy, name))
      return 5;
  using Size = DWORD(WINAPI *)(LPCWSTR, LPDWORD);
  using Read = BOOL(WINAPI *)(LPCWSTR, DWORD, DWORD, LPVOID);
  auto path = std::wstring(system) + L"\\kernel32.dll";
  auto size = reinterpret_cast<Size>(
      GetProcAddress(proxy, "GetFileVersionInfoSizeW"))(path.c_str(), nullptr);
  auto expected = reinterpret_cast<Size>(
      GetProcAddress(real, "GetFileVersionInfoSizeW"))(path.c_str(), nullptr);
  if (!size || size != expected)
    return 6;
  std::vector<unsigned char> actual(size), baseline(size);
  if (!reinterpret_cast<Read>(GetProcAddress(proxy, "GetFileVersionInfoW"))(
          path.c_str(), 0, size, actual.data()) ||
      !reinterpret_cast<Read>(GetProcAddress(real, "GetFileVersionInfoW"))(
          path.c_str(), 0, size, baseline.data()) ||
      actual != baseline)
    return 7;
  FreeLibrary(proxy);
  FreeLibrary(real);
  std::cout << "Proxy exports and version API forwarding passed\n";
  return 0;
}
