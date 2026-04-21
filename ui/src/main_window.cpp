// xpscenery-ui — MainWindow implementation
#include "main_window.hpp"

#include "dsf_inspector_view.hpp"
#include "obj_viewer_view.hpp"
#include "project_view.hpp"
#include "raster_viewer_view.hpp"
#include "tile_grid_view.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QDockWidget>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

namespace xps::ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("xpscenery — MVP Inspector (Fáze 2A)"));
    resize(1200, 800);
    setAcceptDrops(true);

    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(true);
    tabs_->setMovable(true);

    dsf_view_     = new DsfInspectorView(tabs_);
    raster_view_  = new RasterViewerView(tabs_);
    obj_view_     = new ObjViewerView(tabs_);
    project_view_ = new ProjectView(tabs_);
    map_view_     = new TileGridView(tabs_);

    tabs_->addTab(dsf_view_,     tr("DSF Inspector"));
    tabs_->addTab(raster_view_,  tr("Raster (GeoTIFF)"));
    tabs_->addTab(obj_view_,     tr("OBJ8 Viewer"));
    tabs_->addTab(project_view_, tr("Project"));

    // Wrap the map widget with a tiny toolbar (Fit / Clear AOI / Zoom in-out).
    auto* map_panel = new QWidget(tabs_);
    {
        auto* tb = new QToolBar(map_panel);
        tb->setIconSize(QSize(16, 16));
        auto* a_fit  = tb->addAction(style()->standardIcon(QStyle::SP_DirHomeIcon), tr("Fit world"));
        auto* a_clr  = tb->addAction(style()->standardIcon(QStyle::SP_DialogResetButton), tr("Smazat AOI"));
        auto* a_clrR = tb->addAction(tr("Smazat raster bbox"));
        tb->addSeparator();
        auto* a_zin  = tb->addAction(QStringLiteral("+"));
        auto* a_zout = tb->addAction(QStringLiteral("−"));
        connect(a_fit,  &QAction::triggered, map_view_, &TileGridView::reset_view);
        connect(a_clr,  &QAction::triggered, map_view_, &TileGridView::clear_aoi);
        connect(a_clrR, &QAction::triggered, map_view_, &TileGridView::clear_raster_bbox);
        connect(a_zin,  &QAction::triggered, this, [this]{
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Plus, Qt::NoModifier);
            QApplication::sendEvent(map_view_, &e);
        });
        connect(a_zout, &QAction::triggered, this, [this]{
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Minus, Qt::NoModifier);
            QApplication::sendEvent(map_view_, &e);
        });
        auto* vbox = new QVBoxLayout(map_panel);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(0);
        vbox->addWidget(tb);
        vbox->addWidget(map_view_, 1);
    }
    tabs_->addTab(map_panel, tr("Mapa"));

    setCentralWidget(tabs_);

    auto forward = [this](const QString& lvl, const QString& m) { append_log(lvl, m); };
    connect(dsf_view_,     &DsfInspectorView::log, this, forward);
    connect(raster_view_,  &RasterViewerView::log, this, forward);
    connect(obj_view_,     &ObjViewerView::log,    this, forward);
    connect(project_view_, &ProjectView::log,      this, forward);
    connect(map_view_,     &TileGridView::log,     this, forward);

    // Map ↔ Project synchronisation.
    connect(map_view_,    &TileGridView::tile_clicked,
            project_view_, &ProjectView::set_tile);
    connect(map_view_,    &TileGridView::aoi_changed,
            project_view_, &ProjectView::set_aoi);
    connect(project_view_, &ProjectView::tile_changed,
            map_view_,     &TileGridView::set_highlighted_tile);
    connect(project_view_, &ProjectView::aoi_loaded,
            map_view_,     &TileGridView::set_aoi);
    connect(raster_view_,  &RasterViewerView::raster_bbox,
            map_view_,     &TileGridView::set_raster_bbox);

    load_recent();
    build_menus();
    build_toolbar();
    build_status_bar();
    apply_dark_theme();
    restore_settings();
}

MainWindow::~MainWindow() = default;

// ----- File kind sniffing ----------------------------------------------------

MainWindow::FileKind MainWindow::detect_kind(const QString& path) {
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == QLatin1String("dsf"))                              return FileKind::Dsf;
    if (ext == QLatin1String("tif") || ext == QLatin1String("tiff")) return FileKind::GeoTiff;
    if (ext == QLatin1String("obj"))                              return FileKind::Obj;
    if (ext == QLatin1String("xpsproj") || ext == QLatin1String("json")) return FileKind::Project;
    return FileKind::Unknown;
}

bool MainWindow::open_path(const QString& path) {
    if (path.isEmpty()) return false;
    if (!QFileInfo::exists(path)) {
        append_log(QStringLiteral("ERROR"), tr("Soubor neexistuje: %1").arg(path));
        return false;
    }
    switch (detect_kind(path)) {
        case FileKind::Dsf:
            tabs_->setCurrentWidget(dsf_view_);
            dsf_view_->open_file(path);
            break;
        case FileKind::GeoTiff:
            tabs_->setCurrentWidget(raster_view_);
            raster_view_->open_file(path);
            break;
        case FileKind::Obj:
            tabs_->setCurrentWidget(obj_view_);
            obj_view_->open_file(path);
            break;
        case FileKind::Project:
            tabs_->setCurrentWidget(project_view_);
            project_view_->open_file(path);
            setWindowTitle(tr("xpscenery — %1").arg(QFileInfo(path).fileName()));
            break;
        case FileKind::Unknown:
            append_log(QStringLiteral("WARN"),
                tr("Neznámý typ souboru (přípona): %1").arg(QFileInfo(path).fileName()));
            return false;
    }
    push_recent(path);
    return true;
}

// ----- Menus -----------------------------------------------------------------

void MainWindow::build_menus() {
    auto* file_menu = menuBar()->addMenu(tr("&File"));

    auto* open_dsf = file_menu->addAction(tr("Otevřít &DSF…"));
    open_dsf->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    connect(open_dsf, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít DSF"), {}, tr("X-Plane DSF (*.dsf)"));
        if (!f.isEmpty()) open_path(f);
    });

    auto* open_tif = file_menu->addAction(tr("Otevřít &GeoTIFF…"));
    open_tif->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    connect(open_tif, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít GeoTIFF"), {}, tr("GeoTIFF (*.tif *.tiff)"));
        if (!f.isEmpty()) open_path(f);
    });

    auto* open_obj = file_menu->addAction(tr("Otevřít &OBJ…"));
    open_obj->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    connect(open_obj, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít OBJ"), {}, tr("X-Plane OBJ (*.obj)"));
        if (!f.isEmpty()) open_path(f);
    });

    file_menu->addSeparator();
    auto* new_proj = file_menu->addAction(tr("&Nový projekt"));
    new_proj->setShortcut(QKeySequence::New);
    connect(new_proj, &QAction::triggered, this, [this]{
        project_view_->new_project();
        tabs_->setCurrentWidget(project_view_);
        setWindowTitle(tr("xpscenery — (nový projekt)"));
    });
    auto* open_proj = file_menu->addAction(tr("Otevřít &projekt…"));
    open_proj->setShortcut(QKeySequence::Open);
    connect(open_proj, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít projekt"), {}, tr("xpscenery projekt (*.xpsproj *.json)"));
        if (!f.isEmpty()) open_path(f);
    });
    auto* save_proj = file_menu->addAction(tr("&Uložit projekt jako…"));
    save_proj->setShortcut(QKeySequence::Save);
    connect(save_proj, &QAction::triggered, this, [this]{
        project_view_->save_as();
        tabs_->setCurrentWidget(project_view_);
    });

    file_menu->addSeparator();
    recent_menu_ = file_menu->addMenu(tr("Ne&dávné soubory"));
    refresh_recent_menu();

    file_menu->addSeparator();
    auto* quit = file_menu->addAction(tr("U&končit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QMainWindow::close);

    auto* help_menu = menuBar()->addMenu(tr("&Help"));
    auto* about = help_menu->addAction(tr("&O aplikaci xpscenery"));
    connect(about, &QAction::triggered, [this]() {
        QMessageBox::about(this, tr("O aplikaci xpscenery"),
            tr("<b>xpscenery</b> — modern X-Plane scenery toolchain<br/>"
               "Fáze 2A MVP UI (Qt 6.10.2)<br/><br/>"
               "Inspector taby nad io_dsf / io_raster / io_obj / io_config."
               "<br/><br/>© 2026 SimulatorsCzech · MIT License"));
    });
    auto* about_qt = help_menu->addAction(tr("O &Qt…"));
    connect(about_qt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::build_toolbar() {
    toolbar_ = addToolBar(tr("Main"));
    toolbar_->setObjectName(QStringLiteral("MainToolBar"));
    toolbar_->setMovable(true);
    toolbar_->setIconSize(QSize(20, 20));

    const auto mk = [this](QStyle::StandardPixmap icon,
                           const QString& text, const QString& tip) {
        auto* act = new QAction(style()->standardIcon(icon), text, this);
        act->setToolTip(tip);
        toolbar_->addAction(act);
        return act;
    };

    auto* a_dsf = mk(QStyle::SP_FileIcon,
                    tr("DSF"), tr("Otevřít .dsf soubor (Ctrl+D)"));
    connect(a_dsf, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít DSF"), {}, tr("X-Plane DSF (*.dsf)"));
        if (!f.isEmpty()) open_path(f);
    });

    auto* a_tif = mk(QStyle::SP_DriveNetIcon,
                    tr("TIFF"), tr("Otevřít .tif/.tiff (Ctrl+T)"));
    connect(a_tif, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít GeoTIFF"), {}, tr("GeoTIFF (*.tif *.tiff)"));
        if (!f.isEmpty()) open_path(f);
    });

    auto* a_obj = mk(QStyle::SP_FileDialogDetailedView,
                    tr("OBJ"), tr("Otevřít .obj (Ctrl+B)"));
    connect(a_obj, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít OBJ"), {}, tr("X-Plane OBJ (*.obj)"));
        if (!f.isEmpty()) open_path(f);
    });

    toolbar_->addSeparator();
    auto* a_proj = mk(QStyle::SP_DirOpenIcon,
                     tr("Projekt"), tr("Otevřít .xpsproj (Ctrl+O)"));
    connect(a_proj, &QAction::triggered, [this]() {
        const QString f = QFileDialog::getOpenFileName(
            this, tr("Otevřít projekt"), {}, tr("xpscenery projekt (*.xpsproj *.json)"));
        if (!f.isEmpty()) open_path(f);
    });
}

// ----- Status bar + log ------------------------------------------------------

void MainWindow::build_status_bar() {
    auto* dock = new QDockWidget(tr("Log"), this);
    dock->setObjectName(QStringLiteral("LogDock"));
    log_edit_ = new QPlainTextEdit(dock);
    log_edit_->setReadOnly(true);
    log_edit_->setMaximumBlockCount(5000);
    log_edit_->setFont(QFont(QStringLiteral("Consolas"), 9));
    dock->setWidget(log_edit_);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    status_label_ = new QLabel(tr("Připraveno"), this);
    statusBar()->addPermanentWidget(status_label_);
}

// ----- Theme -----------------------------------------------------------------

void MainWindow::apply_dark_theme() {
    const QString qss = QStringLiteral(R"QSS(
        QWidget        { background: #1e1f22; color: #e6e6e6;
                         font-family: "Segoe UI", sans-serif; font-size: 10pt; }
        QTabWidget::pane { border: 1px solid #333; top: -1px; }
        QTabBar::tab   { background: #2b2d30; padding: 6px 12px;
                         border: 1px solid #333; border-bottom: none;
                         border-top-left-radius: 4px; border-top-right-radius: 4px; }
        QTabBar::tab:selected { background: #3a3d42; }
        QLineEdit, QTextBrowser, QPlainTextEdit,
        QTreeWidget, QTableWidget, QListWidget, QSpinBox, QDoubleSpinBox {
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
        QToolBar   { background: #2b2d30; border: none; spacing: 4px; padding: 2px; }
        QToolButton { background: transparent; border: 1px solid transparent;
                      padding: 4px 8px; border-radius: 3px; }
        QToolButton:hover { background: #3a3d42; border-color: #4a4d52; }
        QToolButton:pressed { background: #0a6cff; }
    )QSS");
    qApp->setStyleSheet(qss);
}

// ----- Settings --------------------------------------------------------------

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

// ----- Recent Files ----------------------------------------------------------

void MainWindow::load_recent() {
    QSettings s(QStringLiteral("xpscenery"), QStringLiteral("xpscenery"));
    recent_files_ = s.value(QStringLiteral("RecentFiles")).toStringList();
    // Drop entries that no longer exist on disk; keep order otherwise.
    QStringList kept;
    kept.reserve(recent_files_.size());
    for (const QString& p : recent_files_) {
        if (QFileInfo::exists(p)) kept << p;
    }
    recent_files_.swap(kept);
}

void MainWindow::save_recent() {
    QSettings s(QStringLiteral("xpscenery"), QStringLiteral("xpscenery"));
    s.setValue(QStringLiteral("RecentFiles"), recent_files_);
}

void MainWindow::refresh_recent_menu() {
    if (!recent_menu_) return;
    recent_menu_->clear();
    if (recent_files_.isEmpty()) {
        auto* empty = recent_menu_->addAction(tr("(prázdné)"));
        empty->setEnabled(false);
        return;
    }
    int idx = 1;
    for (const QString& path : recent_files_) {
        const QString label = QStringLiteral("&%1  %2").arg(idx++).arg(path);
        auto* act = recent_menu_->addAction(label);
        connect(act, &QAction::triggered, this, [this, path]() { open_path(path); });
    }
    recent_menu_->addSeparator();
    recent_clear_ = recent_menu_->addAction(tr("Vymazat seznam"));
    connect(recent_clear_, &QAction::triggered, this, [this]() {
        recent_files_.clear();
        save_recent();
        refresh_recent_menu();
    });
}

void MainWindow::push_recent(const QString& path) {
    const QString abs = QFileInfo(path).absoluteFilePath();
    recent_files_.removeAll(abs);
    recent_files_.prepend(abs);
    while (recent_files_.size() > kRecentMax) recent_files_.removeLast();
    save_recent();
    refresh_recent_menu();
}

// ----- Events ----------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent* ev) {
    save_settings();
    QMainWindow::closeEvent(ev);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* ev) {
    if (ev->mimeData()->hasUrls()) ev->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* ev) {
    const auto urls = ev->mimeData()->urls();
    int opened = 0;
    for (const QUrl& u : urls) {
        if (!u.isLocalFile()) continue;
        if (open_path(u.toLocalFile())) ++opened;
    }
    if (opened > 0) ev->acceptProposedAction();
    append_log(QStringLiteral("INFO"),
        tr("Drag-drop: otevřeno %1 z %2 položek").arg(opened).arg(urls.size()));
}

}  // namespace xps::ui
