// ---------------------------------------------------------------------------
// xpscenery-cli — skeleton entry point
// Phase 1 goal: parity with xptools260/RenderFarm subprocess pipeline.
// ---------------------------------------------------------------------------
#include "xpscenery/xpscenery.h"

#include <cstdio>
#include <cstring>

static int print_version() {
    const auto* v = xps_version();
    std::printf("%s\n", xps_build_info());
    std::printf("  version : %d.%d.%d%s%s\n",
                v->major, v->minor, v->patch,
                v->git_sha ? " (" : "",
                v->git_sha ? v->git_sha : "");
    if (v->git_sha) std::printf(")\n");
    return 0;
}

static int print_help() {
    std::printf(
        "xpscenery-cli — modern X-Plane scenery build tool (skeleton)\n"
        "\n"
        "Usage:\n"
        "  xpscenery-cli --version\n"
        "  xpscenery-cli --help\n"
        "  xpscenery-cli <command> [options]\n"
        "\n"
        "Commands (TODO — to be implemented during Phase 1 port):\n"
        "  build <tile.json>    build one X-Plane tile (equivalent of RenderFarm run)\n"
        "  export <tile> <dsf>  export tile to DSF\n"
        "  inspect <dsf>        inspect an existing DSF file\n"
        "  version              print version\n"
        "  help                 print this help\n"
    );
    return 0;
}

int main(int argc, char** argv) {
    if (xps_init() != XPS_OK) {
        std::fprintf(stderr, "xpscenery: init failed: %s\n", xps_last_error());
        return 1;
    }

    int rc = 0;
    if (argc < 2) {
        rc = print_help();
    } else if (std::strcmp(argv[1], "--version") == 0 ||
               std::strcmp(argv[1], "version")   == 0) {
        rc = print_version();
    } else if (std::strcmp(argv[1], "--help") == 0 ||
               std::strcmp(argv[1], "-h")     == 0 ||
               std::strcmp(argv[1], "help")   == 0) {
        rc = print_help();
    } else {
        std::fprintf(stderr, "xpscenery: unknown command '%s'\n", argv[1]);
        std::fprintf(stderr, "Try: xpscenery-cli --help\n");
        rc = 2;
    }

    xps_shutdown();
    return rc;
}
