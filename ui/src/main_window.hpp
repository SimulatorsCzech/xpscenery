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
#include <QStringList>

class QAction;
class QMenu;
class QTabWidget;
class QPlainTextEdit;
class QLabel;
class QToolBar;

namespace xps::ui {

class DsfInspectorView;
class RasterViewerView;
class ObjViewerView;
class ProjectView;
class ShpViewerView;
class PbfViewerView;
class MeshViewerView;
class TileGridView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* ev) override;
    void dragEnterEvent(QDragEnterEvent* ev) override;
    void dropEvent(QDropEvent* ev) override;

private:
    enum class FileKind { Unknown, Dsf, GeoTiff, Obj, Project, Shp, Pbf };

    void build_menus();
    void build_toolbar();
    void build_status_bar();
    void apply_dark_theme();
    void restore_settings();
    void save_settings();
    void append_log(const QString& level, const QString& msg);

    // Unified dispatch: sniff extension, route to right view, update
    // recent files list.  Returns true on success.
    bool open_path(const QString& path);
    static FileKind detect_kind(const QString& path);

    // Recent Files (QSettings persist, capped at 10).
    void load_recent();
    void save_recent();
    void refresh_recent_menu();
    void push_recent(const QString& path);

    QTabWidget*       tabs_           = nullptr;
    DsfInspectorView* dsf_view_       = nullptr;
    RasterViewerView* raster_view_    = nullptr;
    ObjViewerView*    obj_view_       = nullptr;
    ProjectView*      project_view_   = nullptr;
    ShpViewerView*    shp_view_       = nullptr;
    PbfViewerView*    pbf_view_       = nullptr;
    MeshViewerView*   mesh_view_      = nullptr;
    TileGridView*     map_view_       = nullptr;

    QPlainTextEdit*   log_edit_       = nullptr;
    QLabel*           status_label_   = nullptr;

    QToolBar*         toolbar_        = nullptr;
    QMenu*            recent_menu_    = nullptr;
    QAction*          recent_clear_   = nullptr;
    QStringList       recent_files_;

    static constexpr int kRecentMax = 10;
};

}  // namespace xps::ui
