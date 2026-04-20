#include "xpscenery/io_dsf/dsf_strings.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <bit>
#include <cstring>
#include <format>

namespace xps::io_dsf {

namespace {

std::uint32_t le_u32(const std::byte* p) noexcept {
    std::uint32_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    if constexpr (std::endian::native == std::endian::big) {
        v = std::byteswap(v);
    }
    return v;
}

std::string tag_from_id(std::uint32_t raw) noexcept {
    char t[4];
    t[0] = static_cast<char>((raw >> 24) & 0xFF);
    t[1] = static_cast<char>((raw >> 16) & 0xFF);
    t[2] = static_cast<char>((raw >>  8) & 0xFF);
    t[3] = static_cast<char>( raw        & 0xFF);
    for (auto& c : t) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x20 || u > 0x7E) c = '?';
    }
    return std::string{t, 4};
}

// Integer value of the 4 ASCII bytes in the file, parsed as LE u32.
// File bytes for 'HEAD' atom id are 'D','A','E','H' (LE storage of the
// multi-char literal 'HEAD' on little-endian hosts). LE decode gives
// 0x48454144. Same logic for 'PROP' -> file bytes 'P','O','R','P' ->
// 0x504F5250.
constexpr std::uint32_t kHeadId =
    (static_cast<std::uint32_t>('H') << 24) |
    (static_cast<std::uint32_t>('E') << 16) |
    (static_cast<std::uint32_t>('A') <<  8) |
     static_cast<std::uint32_t>('D');
constexpr std::uint32_t kPropId =
    (static_cast<std::uint32_t>('P') << 24) |
    (static_cast<std::uint32_t>('R') << 16) |
    (static_cast<std::uint32_t>('O') <<  8) |
     static_cast<std::uint32_t>('P');

constexpr std::uint32_t make_id4(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24) |
           (static_cast<std::uint32_t>(b) << 16) |
           (static_cast<std::uint32_t>(c) <<  8) |
            static_cast<std::uint32_t>(d);
}
constexpr std::uint32_t kDefnId = make_id4('D','E','F','N');
constexpr std::uint32_t kTertId = make_id4('T','E','R','T');
constexpr std::uint32_t kObjtId = make_id4('O','B','J','T');
constexpr std::uint32_t kPolyId = make_id4('P','O','L','Y');
constexpr std::uint32_t kNetwId = make_id4('N','E','T','W');
constexpr std::uint32_t kDemnId = make_id4('D','E','M','N');

}  // namespace

std::expected<std::vector<TopLevelAtom>, std::string>
read_child_atoms(const std::filesystem::path& dsf_path,
                 const TopLevelAtom& parent)
{
    if (parent.size < 8) {
        return std::unexpected(std::format(
            "DSF parent atom '{}' has size {} < 8", parent.tag, parent.size));
    }
    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));

    const std::uint64_t end = parent.offset + parent.size;
    if (end > bytes->size()) {
        return std::unexpected(std::format(
            "DSF parent atom '{}' extends beyond file (end {}, file size {})",
            parent.tag, end, bytes->size()));
    }

    std::vector<TopLevelAtom> children;
    std::uint64_t pos = parent.offset + 8;
    while (pos + 8 <= end) {
        const auto id   = le_u32(bytes->data() + pos);
        const auto size = le_u32(bytes->data() + pos + 4);
        if (size < 8 || pos + size > end) {
            return std::unexpected(std::format(
                "DSF malformed child atom inside '{}' at offset {} "
                "(size {}, end {})",
                parent.tag, pos, size, end));
        }
        children.push_back(TopLevelAtom{
            .tag    = tag_from_id(id),
            .raw_id = id,
            .offset = pos,
            .size   = size,
        });
        pos += size;
    }
    return children;
}

std::expected<std::vector<std::string>, std::string>
read_string_table(const std::filesystem::path& dsf_path,
                  const TopLevelAtom& atom)
{
    auto bytes = xps::io_filesystem::read_binary(dsf_path);
    if (!bytes) return std::unexpected(std::move(bytes.error()));

    if (atom.size < 8) {
        return std::unexpected(std::format(
            "DSF string-table atom '{}' has size {} < 8",
            atom.tag, atom.size));
    }
    const std::uint64_t payload_begin = atom.offset + 8;
    const std::uint64_t payload_end   = atom.offset + atom.size;
    if (payload_end > bytes->size()) {
        return std::unexpected(std::format(
            "DSF string-table atom '{}' extends past file end", atom.tag));
    }

    std::vector<std::string> out;
    const auto* p  = reinterpret_cast<const char*>(bytes->data() + payload_begin);
    const auto* e  = reinterpret_cast<const char*>(bytes->data() + payload_end);
    const auto* cursor = p;
    while (cursor < e) {
        const auto* nul = cursor;
        while (nul < e && *nul != '\0') ++nul;
        if (nul == e) {
            // Unterminated trailing string — real-world DSFs shouldn't
            // produce this, but we tolerate it for robustness.
            out.emplace_back(cursor, nul);
            break;
        }
        out.emplace_back(cursor, nul);
        cursor = nul + 1;
    }
    // X-Plane writers sometimes emit one trailing empty string from the
    // terminating NUL of the previous entry. Trim that artefact.
    if (!out.empty() && out.back().empty()) {
        out.pop_back();
    }
    return out;
}

std::expected<PropertyList, std::string>
read_properties(const std::filesystem::path& dsf_path)
{
    auto atoms = read_top_level_atoms(dsf_path);
    if (!atoms) return std::unexpected(std::move(atoms.error()));

    const TopLevelAtom* head = nullptr;
    for (const auto& a : *atoms) {
        if (a.raw_id == kHeadId) { head = &a; break; }
    }
    if (head == nullptr) {
        return PropertyList{};  // no HEAD — return empty list, not an error
    }

    auto children = read_child_atoms(dsf_path, *head);
    if (!children) return std::unexpected(std::move(children.error()));

    const TopLevelAtom* prop = nullptr;
    for (const auto& c : *children) {
        if (c.raw_id == kPropId) { prop = &c; break; }
    }
    if (prop == nullptr) {
        return PropertyList{};
    }

    auto strings = read_string_table(dsf_path, *prop);
    if (!strings) return std::unexpected(std::move(strings.error()));

    PropertyList out;
    out.reserve(strings->size() / 2);
    for (std::size_t i = 0; i + 1 < strings->size(); i += 2) {
        out.emplace_back(std::move((*strings)[i]), std::move((*strings)[i + 1]));
    }
    return out;
}

std::expected<std::vector<std::string>, std::string>
read_defn_strings(const std::filesystem::path& dsf_path, DefnKind kind)
{
    std::uint32_t child_id = 0;
    switch (kind) {
        case DefnKind::terrain_types: child_id = kTertId; break;
        case DefnKind::object_defs:   child_id = kObjtId; break;
        case DefnKind::polygon_defs:  child_id = kPolyId; break;
        case DefnKind::network_defs:  child_id = kNetwId; break;
        case DefnKind::raster_names:  child_id = kDemnId; break;
    }

    auto atoms = read_top_level_atoms(dsf_path);
    if (!atoms) return std::unexpected(std::move(atoms.error()));

    const TopLevelAtom* defn = nullptr;
    for (const auto& a : *atoms) {
        if (a.raw_id == kDefnId) { defn = &a; break; }
    }
    if (defn == nullptr) {
        return std::vector<std::string>{};
    }

    auto children = read_child_atoms(dsf_path, *defn);
    if (!children) return std::unexpected(std::move(children.error()));

    const TopLevelAtom* target = nullptr;
    for (const auto& c : *children) {
        if (c.raw_id == child_id) { target = &c; break; }
    }
    if (target == nullptr) {
        return std::vector<std::string>{};
    }

    return read_string_table(dsf_path, *target);
}

}  // namespace xps::io_dsf
