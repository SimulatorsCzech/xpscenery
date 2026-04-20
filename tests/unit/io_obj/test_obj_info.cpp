#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_filesystem/file_io.hpp"
#include "xpscenery/io_obj/obj_info.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace
{

    std::filesystem::path write_tmp_text(std::string_view name,
                                         std::string_view text)
    {
        auto dir = std::filesystem::temp_directory_path() / "xpscenery_tests";
        std::filesystem::create_directories(dir);
        auto p = dir / std::filesystem::path{std::string{name}};
        std::filesystem::remove(p);
        auto bytes = std::span<const std::byte>{
            reinterpret_cast<const std::byte *>(text.data()), text.size()};
        auto r = xps::io_filesystem::write_binary_atomic(p, bytes);
        REQUIRE(r.has_value());
        return p;
    }

} // namespace

TEST_CASE("looks_like_obj8 accepts a minimal OBJ8 preamble",
          "[io_obj]")
{
    const std::string_view src =
        "I\n"
        "800\n"
        "OBJ\n\n"
        "TEXTURE test.png\n"
        "POINT_COUNTS 0 0 0 0\n";
    auto p = write_tmp_text("min.obj", src);
    REQUIRE(xps::io_obj::looks_like_obj8(p));
}

TEST_CASE("looks_like_obj8 rejects a non-OBJ text file", "[io_obj]")
{
    auto p = write_tmp_text("notobj.txt", "hello world\n");
    REQUIRE_FALSE(xps::io_obj::looks_like_obj8(p));
}

TEST_CASE("read_obj_info decodes header and counts", "[io_obj]")
{
    const std::string_view src =
        "# comment ignored\n"
        "I\n"
        "850\n"
        "OBJ\n"
        "\n"
        "TEXTURE wings.png\n"
        "TEXTURE_LIT wings_LIT.png\n"
        "TEXTURE_NORMAL wings_NML.png\n"
        "POINT_COUNTS 120 5 7 360\n"
        "ANIM_begin\n"
        "TRIS 0 360\n"
        "ANIM_end\n"
        "LINES 0 10\n"
        "LIGHTS 0 7\n"
        "TRIS 360 60\n";
    auto p = write_tmp_text("real.obj", src);
    auto info = xps::io_obj::read_obj_info(p);
    REQUIRE(info.has_value());
    REQUIRE(info->platform == xps::io_obj::Platform::ibm);
    REQUIRE(info->version == 850);
    REQUIRE(info->texture == "wings.png");
    REQUIRE(info->texture_lit == "wings_LIT.png");
    REQUIRE(info->texture_normal == "wings_NML.png");
    REQUIRE(info->vt_count == 120);
    REQUIRE(info->vline_count == 5);
    REQUIRE(info->vlight_count == 7);
    REQUIRE(info->idx_count == 360);
    REQUIRE(info->tris_commands == 2);
    REQUIRE(info->lines_commands == 1);
    REQUIRE(info->lights_commands == 1);
    REQUIRE(info->anim_begin == 1);
}

TEST_CASE("read_obj_info rejects an invalid preamble", "[io_obj]")
{
    auto p = write_tmp_text("junk.obj", "not an obj\n");
    auto info = xps::io_obj::read_obj_info(p);
    REQUIRE_FALSE(info.has_value());
}
