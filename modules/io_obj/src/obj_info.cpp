#include "xpscenery/io_obj/obj_info.hpp"
#include "xpscenery/io_filesystem/file_io.hpp"

#include <charconv>
#include <format>
#include <string_view>

namespace xps::io_obj
{

    namespace
    {

        std::string_view strip(std::string_view s) noexcept
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t' ||
                                  s.front() == '\r'))
            {
                s.remove_prefix(1);
            }
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' ||
                                  s.back() == '\r'))
            {
                s.remove_suffix(1);
            }
            return s;
        }

        /// Split `line` at whitespace. Returns first token in `key`, rest in `rest`.
        void split_first(std::string_view line,
                         std::string_view &key, std::string_view &rest) noexcept
        {
            line = strip(line);
            auto n = line.find_first_of(" \t");
            if (n == std::string_view::npos)
            {
                key = line;
                rest = {};
                return;
            }
            key = line.substr(0, n);
            rest = strip(line.substr(n));
        }

        bool parse_u32(std::string_view s, std::uint32_t &out) noexcept
        {
            s = strip(s);
            if (s.empty())
                return false;
            auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
            return ec == std::errc{};
        }

        /// Consume up to three leading non-comment non-blank lines: platform
        /// code, version, "OBJ" magic.
        bool parse_preamble(std::string_view &cursor,
                            Platform &plat, std::uint32_t &version) noexcept
        {
            int phase = 0;
            while (!cursor.empty())
            {
                auto nl = cursor.find('\n');
                auto raw = cursor.substr(0, nl);
                cursor.remove_prefix(nl == std::string_view::npos ? cursor.size()
                                                                  : nl + 1);
                auto line = strip(raw);
                if (line.empty() || line.front() == '#')
                    continue;

                if (phase == 0)
                {
                    if (line == "A")
                        plat = Platform::apple;
                    else if (line == "I")
                        plat = Platform::ibm;
                    else
                        return false;
                    ++phase;
                }
                else if (phase == 1)
                {
                    if (!parse_u32(line, version))
                        return false;
                    ++phase;
                }
                else
                {
                    return line == "OBJ" || line.starts_with("OBJ");
                }
            }
            return false;
        }

    } // namespace

    bool looks_like_obj8(const std::filesystem::path &path)
    {
        auto buf = xps::io_filesystem::read_binary(path);
        if (!buf || buf->size() < 4)
            return false;
        std::string_view text{
            reinterpret_cast<const char *>(buf->data()),
            std::min<std::size_t>(buf->size(), 512)};
        Platform plat = Platform::unknown;
        std::uint32_t ver = 0;
        return parse_preamble(text, plat, ver) && ver >= 700;
    }

    std::expected<ObjInfo, std::string>
    read_obj_info(const std::filesystem::path &path)
    {
        auto buf = xps::io_filesystem::read_binary(path);
        if (!buf)
            return std::unexpected(std::move(buf.error()));
        std::string_view text{
            reinterpret_cast<const char *>(buf->data()), buf->size()};

        ObjInfo info;
        if (!parse_preamble(text, info.platform, info.version))
        {
            return std::unexpected(std::format(
                "'{}' does not start with a valid OBJ8 preamble "
                "(expected A|I / version / OBJ)",
                path.string()));
        }

        while (!text.empty())
        {
            auto nl = text.find('\n');
            auto raw = text.substr(0, nl);
            text.remove_prefix(nl == std::string_view::npos ? text.size()
                                                            : nl + 1);
            auto line = strip(raw);
            if (line.empty() || line.front() == '#')
                continue;

            std::string_view key, rest;
            split_first(line, key, rest);

            if (key == "TEXTURE")
                info.texture = std::string{rest};
            else if (key == "TEXTURE_LIT")
                info.texture_lit = std::string{rest};
            else if (key == "TEXTURE_NORMAL")
                info.texture_normal = std::string{rest};
            else if (key == "POINT_COUNTS")
            {
                // POINT_COUNTS <vt> <vline> <vlight> <idx>
                std::string_view r = rest;
                auto take = [&](std::uint32_t &out)
                {
                    r = strip(r);
                    auto n = r.find_first_of(" \t");
                    auto tok = r.substr(0, n);
                    parse_u32(tok, out);
                    r.remove_prefix(n == std::string_view::npos ? r.size() : n);
                };
                take(info.vt_count);
                take(info.vline_count);
                take(info.vlight_count);
                take(info.idx_count);
            }
            else if (key == "TRIS")
                ++info.tris_commands;
            else if (key == "LINES")
                ++info.lines_commands;
            else if (key == "LIGHTS")
                ++info.lights_commands;
            else if (key == "ANIM_begin")
                ++info.anim_begin;
        }

        return info;
    }

} // namespace xps::io_obj
