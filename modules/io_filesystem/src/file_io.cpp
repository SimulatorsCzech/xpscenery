#include "xpscenery/io_filesystem/file_io.hpp"

#include <cstdio>
#include <cstring>
#include <format>
#include <system_error>
#include <variant>

namespace xps::io_filesystem {

namespace fs = std::filesystem;

namespace {

struct FileHandle {
    std::FILE* f = nullptr;
    ~FileHandle() { if (f) std::fclose(f); }
    FileHandle() = default;
    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&& o) noexcept : f{o.f} { o.f = nullptr; }
    FileHandle& operator=(FileHandle&& o) noexcept {
        if (this != &o) { if (f) std::fclose(f); f = o.f; o.f = nullptr; }
        return *this;
    }
};

FileHandle open_file(const fs::path& path, const char* mode) {
    FileHandle h;
#if defined(_WIN32)
    // Use _wfopen_s to properly handle unicode paths on Windows.
    const std::wstring wpath = path.wstring();
    std::wstring wmode;
    for (const char* p = mode; *p; ++p) wmode.push_back(static_cast<wchar_t>(*p));
    _wfopen_s(&h.f, wpath.c_str(), wmode.c_str());
#else
    h.f = std::fopen(path.c_str(), mode);
#endif
    return h;
}

}  // namespace

std::expected<std::vector<std::byte>, std::string>
read_binary(const fs::path& path) {
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    if (ec) {
        return std::unexpected(std::format(
            "cannot stat {}: {}", path.string(), ec.message()));
    }

    auto handle = open_file(path, "rb");
    if (!handle.f) {
        return std::unexpected(std::format(
            "cannot open {} for reading", path.string()));
    }

    std::vector<std::byte> buffer(static_cast<std::size_t>(size));
    if (size > 0) {
        const auto got = std::fread(buffer.data(), 1, buffer.size(), handle.f);
        if (got != buffer.size()) {
            return std::unexpected(std::format(
                "short read on {}: got {} of {} bytes",
                path.string(), got, buffer.size()));
        }
    }
    return buffer;
}

std::expected<std::string, std::string>
read_text_utf8(const fs::path& path) {
    auto bytes = read_binary(path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));
    std::string s;
    s.resize(bytes->size());
    if (!bytes->empty()) {
        std::memcpy(s.data(), bytes->data(), bytes->size());
    }
    // Strip a UTF-8 BOM if present.
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF
                      && static_cast<unsigned char>(s[1]) == 0xBB
                      && static_cast<unsigned char>(s[2]) == 0xBF) {
        s.erase(0, 3);
    }
    return s;
}

std::expected<std::monostate, std::string>
write_binary_atomic(const fs::path& path, std::span<const std::byte> data) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    // Ignore ec — parent may already exist; fail later on fopen if truly broken.

    auto tmp = path;
    tmp += ".tmp";
    {
        auto handle = open_file(tmp, "wb");
        if (!handle.f) {
            return std::unexpected(std::format(
                "cannot open {} for writing", tmp.string()));
        }
        if (!data.empty()) {
            const auto wrote = std::fwrite(data.data(), 1, data.size(), handle.f);
            if (wrote != data.size()) {
                return std::unexpected(std::format(
                    "short write on {}: wrote {} of {} bytes",
                    tmp.string(), wrote, data.size()));
            }
        }
        std::fflush(handle.f);
    }

    fs::rename(tmp, path, ec);
    if (ec) {
        // POSIX rename replaces atomically; on Windows, same result for files
        // on the same volume. Fall back to remove + rename if needed.
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
        if (ec) {
            return std::unexpected(std::format(
                "atomic rename failed: {} -> {}: {}",
                tmp.string(), path.string(), ec.message()));
        }
    }
    return std::monostate{};
}

std::expected<std::monostate, std::string>
write_text_utf8_atomic(const fs::path& path, std::string_view text) {
    auto bytes = std::span{
        reinterpret_cast<const std::byte*>(text.data()), text.size()};
    return write_binary_atomic(path, bytes);
}

}  // namespace xps::io_filesystem
