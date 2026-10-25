#include "path.h"
#include <MinHook.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <xml_operations.h>

namespace {
using Bytes = std::vector<unsigned char>;
using Open = bool (*)(void *, void *, unsigned);
using Close = void (*)(void *);
Open original_open;
Close original_close;
unsigned char *image;
std::ofstream log_file;
std::mutex lock;
struct Patch {
  fs::path root, relative;
  Bytes bytes;
  bool modops;
};
std::map<std::string, std::vector<Patch>> plans;
std::map<std::string, std::string> variables;
struct Backing {
  std::array<unsigned char, 16> original;
  Bytes bytes;
};
std::unordered_map<void *, Backing> backing;
std::map<std::string, Bytes> cache;
std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  std::replace(s.begin(), s.end(), '\\', '/');
  return s;
}
std::wstring env(const wchar_t *name) {
  DWORD n = GetEnvironmentVariableW(name, nullptr, 0);
  if (!n)
    return {};
  std::wstring value(n, L'\0');
  GetEnvironmentVariableW(name, value.data(), n);
  value.resize(n - 1);
  return value;
}
Bytes read(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("cannot read " + path.u8string());
  return Bytes(std::istreambuf_iterator<char>(input), {});
}
[[noreturn]] void fail(const std::string &message) {
  log_file << "error " << message << std::endl;
  MessageBoxA(nullptr, message.c_str(), "Anno 2070 modloader",
              MB_OK | MB_ICONERROR);
  ExitProcess(1);
}
void restore(void *file) {
  auto it = backing.find(file);
  if (it != backing.end()) {
    std::memcpy(static_cast<char *>(file) + 0x20, it->second.original.data(),
                16);
    backing.erase(it);
  }
}
void close_file(void *file) {
  {
    std::lock_guard<std::mutex> guard(lock);
    restore(file);
  }
  original_close(file);
}
// Check all includes before invoking the upstream reader (which follows disk
// paths).
void check_includes(const fs::path &file, const fs::path &root,
                    std::set<fs::path> &active, unsigned depth = 0) {
  const auto actual = fs::canonical(file);
  auto relative = actual.lexically_relative(fs::canonical(root));
  if (relative.empty() || *relative.begin() == ".." || depth > 32 ||
      !active.insert(actual).second)
    throw std::runtime_error(
        "include escapes package, cycles, or exceeds depth: " +
        file.u8string());
  pugi::xml_document doc;
  if (!doc.load_file(file.c_str()) ||
      lower(doc.document_element().name()) != "modops")
    throw std::runtime_error("invalid patch XML: " + file.u8string());
  std::function<void(pugi::xml_node)> walk = [&](pugi::xml_node node) {
    if (lower(node.name()) == "include") {
      auto include = node.attribute("File").as_string();
      if (!*include)
        throw std::runtime_error("Include requires File");
      std::string value(include);
      auto nested = value.front() == '/'
                        ? root / fs::u8path(value.substr(1))
                        : file.parent_path() / fs::u8path(value);
      check_includes(nested, root, active, depth + 1);
    }
    for (auto child : node.children())
      walk(child);
  };
  walk(doc);
  active.erase(actual);
}
struct Writer : pugi::xml_writer {
  Bytes bytes;
  void write(const void *data, size_t size) override {
    auto p = static_cast<const unsigned char *>(data);
    bytes.insert(bytes.end(), p, p + size);
  }
};
Bytes compose(const std::string &path, Bytes bytes) {
  for (const auto &patch : plans.at(path)) {
    if (!patch.modops) {
      bytes = patch.bytes;
      continue;
    }
    auto doc = std::make_shared<pugi::xml_document>();
    auto parsed =
        doc->load_buffer(bytes.data(), bytes.size(),
                         pugi::parse_default | pugi::parse_declaration |
                             pugi::parse_comments | pugi::parse_pi);
    if (!parsed)
      throw std::runtime_error(path + ": " + parsed.description());
    std::set<fs::path> includes;
    check_includes(patch.root / patch.relative, patch.root, includes);
    std::ostringstream diagnostics;
    auto logger = std::make_shared<spdlog::logger>(
        "modops",
        std::make_shared<spdlog::sinks::ostream_sink_mt>(diagnostics));
    logger->set_level(spdlog::level::warn);
    struct LoggerScope {
      std::shared_ptr<spdlog::logger> previous = spdlog::default_logger();
      ~LoggerScope() { spdlog::set_default_logger(previous); }
    } logger_scope;
    spdlog::set_default_logger(logger);
    auto operations = xmlops::XmlOperation::GetXmlOperationsFromFile(
        patch.root / patch.relative, patch.root.filename().u8string(),
        patch.root.filename().u8string(), &variables, path, patch.root);
    if (operations.empty())
      throw std::runtime_error("patch contains no operations");
    for (auto &operation : operations)
      operation.Apply(doc, xmlops::XmlPatchType::Assets, false);
    if (!diagnostics.str().empty())
      throw std::runtime_error(diagnostics.str());
    unsigned flags = pugi::format_raw;
    if ((bytes.size() >= 3 && bytes[0] == 0xef && bytes[1] == 0xbb &&
         bytes[2] == 0xbf) ||
        (bytes.size() >= 2 && ((bytes[0] == 0xff && bytes[1] == 0xfe) ||
                               (bytes[0] == 0xfe && bytes[1] == 0xff))))
      flags |= pugi::format_write_bom;
    if (doc->first_child().type() != pugi::node_declaration)
      flags |= pugi::format_no_declaration;
    Writer writer;
    doc->save(writer, "", flags, parsed.encoding);
    bytes = std::move(writer.bytes);
  }
  if (bytes.size() > UINT32_MAX)
    throw std::runtime_error("replacement exceeds native file size");
  if (bytes.empty())
    bytes.reserve(1);
  return bytes;
}
bool open_file(void *file, void *string, unsigned flags) {
  {
    std::lock_guard<std::mutex> guard(lock);
    restore(file);
  }
  bool success = original_open(file, string, flags);
  try {
    auto raw = static_cast<unsigned char *>(string);
    size_t length, capacity;
    std::memcpy(&length, raw + 0x10, 8);
    std::memcpy(&capacity, raw + 0x18, 8);
    if (length > 32768 || capacity < length || (flags & 0x8000) == 0 ||
        (flags & 0x4000) != 0)
      return success;
    auto data = capacity >= 8 ? *reinterpret_cast<wchar_t **>(raw)
                              : reinterpret_cast<wchar_t *>(raw);
    const auto path =
        anno2070::normalize_game_path(fs::path(std::wstring(data, length)));
    if (plans.find(path) == plans.end())
      return success;
    std::lock_guard<std::mutex> guard(lock);
    auto cached = cache.find(path);
    if (cached == cache.end()) {
      Bytes source;
      std::array<unsigned char, 16> source_fields;
      std::memcpy(source_fields.data(), static_cast<char *>(file) + 0x20, 16);
      if (success) {
        auto size =
            reinterpret_cast<unsigned (*)(void *)>(image + 0x77bca0)(file);
        if (size > 64 * 1024 * 1024)
          throw std::runtime_error("baseline exceeds 64MiB");
        source.resize(size);
        auto count = reinterpret_cast<unsigned (*)(void *, void *, unsigned)>(
            image + 0x77ba70)(file, source.data(), size);
        std::memcpy(static_cast<char *>(file) + 0x20, source_fields.data(), 16);
        if (count != size)
          throw std::runtime_error("short baseline read");
      }
      cached = cache.emplace(path, compose(path, std::move(source))).first;
      log_file << "compose " << path << " bytes=" << cached->second.size()
               << std::endl;
    }
    Backing replacement;
    std::memcpy(replacement.original.data(), static_cast<char *>(file) + 0x20,
                16);
    replacement.bytes = cached->second;
    if (replacement.bytes.empty())
      replacement.bytes.reserve(1);
    auto &owned = backing.emplace(file, std::move(replacement)).first->second;
    *reinterpret_cast<void **>(static_cast<char *>(file) + 0x20) =
        owned.bytes.data();
    *reinterpret_cast<unsigned *>(static_cast<char *>(file) + 0x28) =
        static_cast<unsigned>(owned.bytes.size());
    *reinterpret_cast<unsigned *>(static_cast<char *>(file) + 0x2c) = 0;
    log_file << "open " << path << std::endl;
    return true;
  } catch (const std::exception &e) {
    fail(e.what());
  }
}
void scan(const fs::path &mods) {
  std::vector<fs::path> roots;
  for (const auto &item : fs::directory_iterator(mods))
    if (item.is_directory() &&
        item.path().filename().u8string().front() != '-') {
      if (item.is_symlink())
        throw std::runtime_error("symlink package is unsupported");
      roots.push_back(item.path());
    }
  std::sort(roots.begin(), roots.end());
  for (const auto &root : roots) {
    if (fs::exists(root / "modinfo.json"))
      throw std::runtime_error(
          "modinfo.json dependency ordering is not implemented");
    variables[root.filename().u8string()] = "true";
    log_file << "mod " << root.u8string() << std::endl;
    std::set<std::string> seen;
    for (const auto *folder : {"data", "addondata"}) {
      if (!fs::is_directory(root / folder))
        continue;
      for (const auto &item : fs::recursive_directory_iterator(root / folder)) {
        if (item.is_symlink())
          throw std::runtime_error("symlinks are not supported in mods");
        if (!item.is_regular_file() ||
            item.path().filename().u8string().size() >= 12 &&
                item.path().filename().u8string().substr(
                    item.path().filename().u8string().size() - 12) ==
                    ".include.xml")
          continue;
        auto relative = item.path().lexically_relative(root);
        auto path = anno2070::normalize_game_path(relative);
        if (!seen.insert(path).second)
          throw std::runtime_error("duplicate normalized path " + path);
        auto bytes = read(item.path());
        pugi::xml_document doc;
        bool modops = doc.load_buffer(bytes.data(), bytes.size()) &&
                      lower(doc.document_element().name()) == "modops";
        plans[path].push_back({root, relative, std::move(bytes), modops});
      }
    }
  }
}
} // namespace
void initialize_loader() {
  if (env(L"ANNO5_MODS_DISABLED") == L"1")
    return;
  wchar_t executable[32768];
  GetModuleFileNameW(nullptr, executable, 32768);
  const auto game = fs::path(executable).parent_path();
  auto selected = env(L"ANNO5_MODS_DIR");
  const auto mods = selected.empty() ? game / L"mods" : fs::path(selected);
  log_file.open(game / L"anno2070-modloader.log", std::ios::trunc);
  try {
    log_file << "anno2070-modloader 0.1.0 build=3.00.8051 mods="
             << mods.u8string() << std::endl;
    if (!fs::exists(mods)) {
      if (!selected.empty())
        throw std::runtime_error("mods directory does not exist");
      return;
    }
    scan(mods);
    if (plans.empty())
      return;
    image = reinterpret_cast<unsigned char *>(GetModuleHandleW(nullptr));
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER *>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
      throw std::runtime_error("invalid main PE image");
    auto pe = reinterpret_cast<IMAGE_NT_HEADERS64 *>(image + dos->e_lfanew);
    const unsigned char marker[] = {0x40, 0x53, 0x48, 0x83, 0xec, 0x20,
                                    0x48, 0x8b, 0xd9, 0x48, 0x8b, 0x89,
                                    0x90, 0x01, 0,    0};
    if (pe->Signature != IMAGE_NT_SIGNATURE ||
        pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        pe->OptionalHeader.SizeOfImage != 0x276f000 ||
        std::memcmp(image + 0xc1e310, marker, sizeof(marker)))
      throw std::runtime_error(
          "unsupported main image: requires Anno 2070 build 8051 x64");
    const unsigned char prologue[] = {0x48, 0x89, 0x5c, 0x24, 0x08,
                                      0x48, 0x89, 0x74, 0x24, 0x10};
    const unsigned char close[] = {0x48, 0x89, 0x5c, 0x24, 0x08,
                                   0x48, 0x89, 0x6c, 0x24, 0x10};
    if (std::memcmp(image + 0x77c860, prologue, 10) ||
        std::memcmp(image + 0x77cf90, close, 10) ||
        *reinterpret_cast<void **>(image + 0x16c9958) != image + 0x77b910)
      throw std::runtime_error("CEngineFile invariants differ");
    HMODULE pinned;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_PIN,
                       reinterpret_cast<LPCWSTR>(&initialize_loader), &pinned);
    if (MH_Initialize() != MH_OK ||
        MH_CreateHook(image + 0x77c860, reinterpret_cast<void *>(&open_file),
                      reinterpret_cast<void **>(&original_open)) != MH_OK ||
        MH_CreateHook(image + 0x77cf90, reinterpret_cast<void *>(&close_file),
                      reinterpret_cast<void **>(&original_close)) != MH_OK ||
        MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
      throw std::runtime_error("cannot install CEngineFile detours");
    log_file << "ready paths=" << plans.size() << std::endl;
  } catch (const std::exception &e) {
    fail(e.what());
  }
}
