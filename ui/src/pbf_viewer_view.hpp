#pragma once
// xpscenery-ui — OSM PBF (.osm.pbf) header viewer.
//
// Zobrazuje metadata prvního BlobHeader (typ + datasize) a file size.
// Deep parse OSMHeader (bbox) vyžaduje zlib dekompresi a je na pozdější
// fázi. Uživatel tedy uvidí, že se jedná o validní PBF a jak velký je
// první blob.

#include <QWidget>

class QLineEdit;
class QLabel;
class QTableWidget;

namespace xps::ui {

class PbfViewerView : public QWidget {
    Q_OBJECT
public:
    explicit PbfViewerView(QWidget* parent = nullptr);

signals:
    void log(const QString& level, const QString& msg);

public slots:
    void open_file(const QString& path);

private slots:
    void on_browse();

private:
    void populate(const QString& path);

    QLineEdit*    path_edit_   = nullptr;
    QLabel*       summary_lbl_ = nullptr;
    QTableWidget* kv_table_    = nullptr;
};

}  // namespace xps::ui
