// ---------------------------------------------------------------------------
// xpscenery-ui — Qt 6 entry point
// ---------------------------------------------------------------------------
#include "xpscenery/xpscenery.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>
#include <QLoggingCategory>

int main(int argc, char* argv[]) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // High-DPI is default-enabled on Qt 6; keep explicit for clarity.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QGuiApplication app(argc, argv);
    app.setOrganizationName("xpscenery");
    app.setOrganizationDomain("xpscenery.org");
    app.setApplicationName("xpscenery");
    app.setApplicationVersion(QStringLiteral("0.1.0-alpha.0"));
    app.setWindowIcon(QIcon(":/XpScenery/assets/icon.png"));

    // Modern default style — "Fusion"-ish but QML-native.
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    if (xps_init() != XPS_OK) {
        qCritical("xpscenery core init failed: %s", xps_last_error());
        return 1;
    }

    QQmlApplicationEngine engine;
    engine.loadFromModule(QStringLiteral("XpScenery"),
                          QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty()) {
        qCritical("Failed to load QML root");
        xps_shutdown();
        return 2;
    }

    const int rc = app.exec();
    xps_shutdown();
    return rc;
}
