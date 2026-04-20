#include <catch2/catch_test_macros.hpp>

#include "xpscenery/io_filesystem/file_io.hpp"

#include <array>
#include <cstddef>
#include <filesystem>

TEST_CASE("write_text_utf8_atomic + read_text_utf8 round-trip",
          "[io_filesystem][file_io]") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "xpscenery_tests";
    fs::create_directories(dir);
    auto file = dir / "hello.txt";
    fs::remove(file);

    const std::string payload = "Příliš žluťoučký kůň úpěl ďábelské ódy.";
    auto wr = xps::io_filesystem::write_text_utf8_atomic(file, payload);
    REQUIRE(wr.has_value());

    auto rd = xps::io_filesystem::read_text_utf8(file);
    REQUIRE(rd.has_value());
    REQUIRE(*rd == payload);
}

TEST_CASE("read_binary reports size", "[io_filesystem][file_io]") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "xpscenery_tests";
    fs::create_directories(dir);
    auto file = dir / "blob.bin";
    fs::remove(file);

    std::array<std::byte, 256> data{};
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::byte>(i);
    }
    auto wr = xps::io_filesystem::write_binary_atomic(file, data);
    REQUIRE(wr.has_value());

    auto rd = xps::io_filesystem::read_binary(file);
    REQUIRE(rd.has_value());
    REQUIRE(rd->size() == data.size());
    for (std::size_t i = 0; i < data.size(); ++i) {
        REQUIRE(static_cast<unsigned>(std::to_integer<unsigned char>((*rd)[i])) == i);
    }
}

TEST_CASE("read_text_utf8 strips BOM", "[io_filesystem][file_io]") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "xpscenery_tests";
    fs::create_directories(dir);
    auto file = dir / "bom.txt";
    fs::remove(file);

    std::array<std::byte, 6> bom_and_hi{
        std::byte{0xEF}, std::byte{0xBB}, std::byte{0xBF},
        std::byte{'h'},  std::byte{'i'},  std::byte{'!'}};
    auto wr = xps::io_filesystem::write_binary_atomic(file, bom_and_hi);
    REQUIRE(wr.has_value());

    auto rd = xps::io_filesystem::read_text_utf8(file);
    REQUIRE(rd.has_value());
    REQUIRE(*rd == "hi!");
}
