#pragma once
// xpscenery — io_osm/pbf_header.hpp
//
// OpenStreetMap Protocolbuffer Binary Format (.osm.pbf) top-level
// BlobHeader parser. Neparsuje samotný Blob (to je zlib-zkomprimovaný
// protobuf OSMHeader/OSMData) — stačí nám první BlobHeader pro ověření
// a metadata.
//
// Struktura file formatu (dle openstreetmap.org/wiki/PBF_Format):
//   [uint32 BE: header_length]
//   [header_length bytes: BlobHeader]   (message: type, indexdata?, datasize)
//   [datasize bytes : Blob]             (zipped OSMHeader / OSMData)
//   (opakuje se)
//
// BlobHeader protobuf schéma (zjednodušené):
//   1: string type          (povinné, "OSMHeader" nebo "OSMData")
//   2: optional bytes indexdata
//   3: int32  datasize      (povinné)

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace xps::io_osm
{

    struct PbfHeaderInfo
    {
        std::uint64_t file_size         = 0; ///< v bajtech
        std::uint32_t first_blob_header_len = 0; ///< BE uint32 prefix
        std::string   first_blob_type;       ///< "OSMHeader" očekáváno
        std::int32_t  first_blob_datasize = 0; ///< velikost prvního Blobu
    };

    /// Přečte prvních několik set bajtů `.pbf` souboru a vrátí info o
    /// prvním BlobHeader. Nerozbaluje Blob (zlib) — to je mimo scope Fáze 2.
    [[nodiscard]] std::expected<PbfHeaderInfo, std::string>
    read_pbf_header(const std::filesystem::path &path);

} // namespace xps::io_osm
