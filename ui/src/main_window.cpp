// xpscenery-ui — MainWindow implementation
#include "main_window.hpp"

#include "dsf_inspector_view.hpp"
#include "obj_viewer_view.hpp"
#include "project_view.hpp"
#include "raster_viewer_view.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QDockWidget>
#include <QToolBar>

namespace xps::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("xpscenery — MVP Inspector (Fáze 2A)"));
    resize(1200, 800);

    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(true);
    tabs_->setMovable(true);

    dsf_view_     = new DsfInspectorView(tabs_);
    raster_view_  = new RasterViewerView(tabs_);
    obj_view_     = new ObjViewerView(tabs_);
    project_view_ = new ProjectView(tabs_);

    tabs_->addTab(dsf_view_,     tr("DSF Inspector"));
    tabs_->addTab(raster_view_,  tr("Raster (GeoTIFF)"));
    tabs_->addTab(obj_view_,     tr("OBJ8 Viewer"));
    tabs_->addTab(project_view_, tr("Project"));

    setCentralWidget(tabs_);

    auto forward = [this](const QString& lvl, const QString& m) { append_log(lvl, m); };
    connect(dsf_view_,     &DsfInspectorView::log, this, forward);
    connect(raster_view_,  &RasterViewerView::log, this, forward);
    connect(obj_view_,     &ObjViewerView::log,    this, forward);
    connect(project_view_, &ProjectView::log,      this, forward);

    build_menus();
    build_status_bar();
    apply_dark_theme();
    restore_settings();
}

MainWindow::~MainWindow() = default;

void MainWindow::build_menus() {
    auto* file_menu = menuBar()->addMenu(tr("&File"));

    auto* open_dsf = file_menu->addAction(tr("Open DSF…"));
    connect(open_dsf, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Open DSF"), {}, tr("X-Plane DSF (*.dsf)"));
        if (!f.isEmpty()) { tabs_->setCurrentWidget(dsf_view_); dsf_view_->open_file(f); }
    });

    auto* open_tif = file_menu->addAction(tr("Open GeoTIFF…"));
    connect(open_tif, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Open GeoTIFF"), {}, tr("GeoTIFF (*.tif *.tiff)"));
        if (!f.isEmpty()) { tabs_->setCurrentWidget(raster_view_); raster_view_->open_file(f); }
    });

    auto* open_obj = file_menu->addAction(tr("Open OBJ…"));
    connect(open_obj, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Open OBJ"), {}, tr("X-Plane OBJ (*.obj)"));
        if (!f.isEmpty()) { tabs_->setCurrentWidget(obj_view_); obj_view_->open_file(f); }
    });

    file_menu->addSeparator();
    auto* open_proj = file_menu->addAction(tr("Open Project…"));
    connect(open_proj, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Open project"), {}, tr("xpscenery project (*.xpsproj *.json)"));
        if (!f.isEmpty()) { tabs_->setCurrentWidget(project_view_); project_view_->open_file(f); }
    });

    file_menu->addSeparator();
    auto* quit = file_menu->addAction(tr("E&xit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QMainWindow::close);

    auto* help_menu = menuBar()->addMenu(tr("&Help"));
    auto* about = help_menu->addAction(tr("&About xpscenery"));
    connect(about, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("About xpscenery"),
            tr("<b>xpscenery</b> — modern X-Plane scenery toolchain<br/>"
               "Fáze 2A MVP UI (Qt 6.10.2)<br/><br/>"
               "Inspector tabs over io_dsf / io_raster / io_obj / io_config."));
    });
}

void MainWindow::build_status_bar() {
    // Docked log panel
    auto* dock = new QDockWidget(tr("Log"), this);
    dock->setObjectName(QStringLiteral("LogDock"));
    log_edit_ = new QPlainTextEdit(dock);
    log_edit_->setReadOnly(true);
    log_edit_->setMaximumBlockCount(5000);
    log_edit_->setFont(QFont(QStringLiteral("Consolas"), 9));
    dock->setWidget(log_edit_);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    status_label_ = new QLabel(tr("Ready"), this);
    statusBar()->addPermanentWidget(status_label_);
}

void MainWindow::apply_dark_theme() {
    // Fluent-lite dark QSS. Kept in-source so first-run has no file
    // dependency; we'll move this to :/styles/dark.qss in Fáze 2B.
    const QString qss = QStringLiteral(R"QSS(
        QWidget        { background: #1e1f22; color: #e6e6e6;
                         font-family: "Segoe UI", sans-serif; font-size: 10pt; }
        QTabWidget::pane { border: 1px solid #333; top: -1px; }
        QTabBar::tab   { background: #2b2d30; padding: 6px 12px;
                         border: 1px solid #333; border-bottom: none;
                         border-top-left-radius: 4px; border-top-right-radius: 4px; }
        QTabBar::tab:selected { background: #3a3d42; }
        QLineEdit, QTextBrowser, QPlainTextEdit,
        QTreeWidget, QTableWidget, QSpinBox, QDoubleSpinBox {
            background: #2b2d30; color: #e6e6e6; border: 1px solid #3a3d42;
            selection-background-color: #0a6cff;
        }
        QPushButton { background: #3a3d42; border: 1px solid #4a4d52;
                      padding: 5px 12px; border-radius: 3px; }
        QPushButton:hover { background: #4a4d52; }
        QPushButton:pressed { background: #0a6cff; }
        QHeaderView::section { background: #2b2d30; color: #e6e6e6;
                               border: 1px solid #3a3d42; padding: 4px; }
        QGroupBox { border: 1px solid #3a3d42; border-radius: 4px;
                    margin-top: 10px; padding-top: 6px; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QStatusBar { background: #2b2d30; }
        QMenuBar   { background: #1e1f22; }
        QMenuBar::item:selected { background: #3a3d42; }
        QMenu      { background: #2b2d30; border: 1px solid #3a3d42; }
        QMenu::item:selected { background: #0a6cff; }
        QDockWidget::title { background: #2b2d30; padding: 4px 8px; }
    )QSS");
    qApp->setStyleSheet(qss);
}

void MainWindow::restore_settings() {
    QSettings s(QStringLiteral("xpscenery"), QStringLiteral("xpscenery"));
    const QByteArray geo = s.value(QStringLiteral("MainWindow/geometry")).toByteArray();
    const QByteArray st  = s.value(QStringLiteral("MainWindow/state")).toByteArray();
    if (!geo.isEmpty()) restoreGeometry(geo);
    if (!st.isEmpty())  restoreState(st);
}

void MainWindow::save_settings() {
    QSettings s(QStringLiteral("xpscenery"), QStringLiteral("xpscenery"));
    s.setValue(QStringLiteral("MainWindow/geometry"), saveGeometry());
    s.setValue(QStringLiteral("MainWindow/state"),    saveState());
}

void MainWindow::append_log(const QString& level, const QString& msg) {
    if (!log_edit_) return;
    const QString ts = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    const QString line = QStringLiteral("[%1] %2: %3").arg(ts, level, msg);
    log_edit_->appendPlainText(line);
    if (status_label_) status_label_->setText(msg);
}

void MainWindow::closeEvent(QCloseEvent* ev) {
    save_settings();
    QMainWindow::closeEvent(ev);
}

}  // namespace xps::ui
