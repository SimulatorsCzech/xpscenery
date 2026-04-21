#pragma once
// GeoTIFF metadata viewer. Uses xps::io_raster::read_geotiff_ifd to pull
// width/height, sample format, compression, ModelPixelScale, tiepoints,
// GeoKeyDirectory from classic TIFF or BigTIFF.

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QLabel;

namespace xps::ui {

class RasterViewerView : public QWidget {
    Q_OBJECT
public:
    explicit RasterViewerView(QWidget* parent = nullptr);

signals:
    void log(const QString& level, const QString& msg);
    /// Emitováno, pokud se podařilo z IFD odvodit geografický bbox
    /// (EPSG:4326 stupně). Mapa to použije jako oranžový overlay.
    void raster_bbox(double west, double south, double east, double north);
    /// Uživatel klikl na "Zobrazit v mapě" — MainWindow přepne tab a
    /// přiblíží se na raster bbox.
    void show_in_map();

public slots:
    void open_file(const QString& path);

private slots:
    void on_browse();

private:
    void populate(const QString& path);

    QLineEdit*    path_edit_   = nullptr;
    QLabel*       summary_lbl_ = nullptr;
    QTableWidget* table_       = nullptr;  ///< GeoKeys
    QTableWidget* tiepoints_   = nullptr;  ///< ModelTiepointTag rows
};

}  // namespace xps::ui
