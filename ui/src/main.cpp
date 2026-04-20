// xpscenery-ui — Qt 6 MVP entry point (Fáze 2A, Qt Widgets)
#include "main_window.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("xpscenery"));
    app.setOrganizationDomain(QStringLiteral("xpscenery.org"));
    app.setApplicationName(QStringLiteral("xpscenery"));
    app.setApplicationVersion(QStringLiteral("0.2.0-alpha.0"));
    // "Fusion" is the only cross-platform style that respects QSS fully.
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    xps::ui::MainWindow w;
    w.show();
    return app.exec();
}
