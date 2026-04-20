#pragma once
// xpscenery-ui — main application window
//
// MVP Fáze 2A: 4 inspector tabs over the read-only backend modules.
//  - DSF Inspector   → xps::io_dsf::read_dsf_blob
//  - Raster Viewer   → xps::io_raster::read_geotiff_ifd
//  - OBJ Viewer      → xps::io_obj::read_obj_info
//  - Project Editor  → xps::io_config::TileConfig (load/save .xpsproj JSON)
//
// Architecture: pure Qt Widgets (no QML) for simplicity. Each view is
// a self-contained QWidget subclass; the main window owns a QTabWidget
// and a shared log sink displayed in the status dock.

#include <QMainWindow>

class QTabWidget;
class QPlainTextEdit;
class QLabel;

namespace xps::ui {

class DsfInspectorView;
class RasterViewerView;
class ObjViewerView;
class ProjectView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* ev) override;

private:
    void build_menus();
    void build_status_bar();
    void apply_dark_theme();
    void restore_settings();
    void save_settings();
    void append_log(const QString& level, const QString& msg);

    QTabWidget*       tabs_           = nullptr;
    DsfInspectorView* dsf_view_       = nullptr;
    RasterViewerView* raster_view_    = nullptr;
    ObjViewerView*    obj_view_       = nullptr;
    ProjectView*      project_view_   = nullptr;

    QPlainTextEdit*   log_edit_       = nullptr;
    QLabel*           status_label_   = nullptr;
};

}  // namespace xps::ui
