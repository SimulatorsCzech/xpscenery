#pragma once
// xpscenery-ui — Shapefile (.shp) header viewer.
//
// Zobrazuje 100-byte header: file_code, version, shape type, file size,
// X/Y/Z/M bounding box a mini-mapu s overlay. (Deep record parsing je
// mimo Fázi 2 — pouze detekce + bbox.)

#include <QWidget>

class QLineEdit;
class QLabel;
class QTableWidget;

namespace xps::ui {

class TileGridView;

class ShpViewerView : public QWidget {
    Q_OBJECT
public:
    explicit ShpViewerView(QWidget* parent = nullptr);

signals:
    void log(const QString& level, const QString& msg);
    /// Emitováno, pokud je bbox v EPSG:4326 rozsahu.
    void shp_bbox(double west, double south, double east, double north);
    void show_in_map();

public slots:
    void open_file(const QString& path);

private slots:
    void on_browse();

private:
    void populate(const QString& path);

    QLineEdit*    path_edit_   = nullptr;
    QLabel*       summary_lbl_ = nullptr;
    QTableWidget* kv_table_    = nullptr;
    TileGridView* mini_map_    = nullptr;
};

}  // namespace xps::ui
