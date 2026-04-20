// xpscenery-cli — thin launcher. All real work lives in modules/app_cli.
//
// Rationale: keeping main.cpp tiny lets tests (and later the GUI) reuse
// the same command dispatch via xps::app_cli::run().
#include "xpscenery/app_cli/cli_app.hpp"
#include "xpscenery/xpscenery.h"

#include <cstdio>

int main(int argc, char** argv) {
    if (xps_init() != XPS_OK) {
        std::fprintf(stderr, "xpscenery: init failed: %s\n", xps_last_error());
        return 1;
    }
    const int rc = xps::app_cli::run(argc, argv);
    xps_shutdown();
    return rc;
}
