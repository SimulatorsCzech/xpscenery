#pragma once
// xpscenery-ui — TileGridView
//
// Lehký mapový náhled postavený na čistém QPainter (bez qtlocation,
// bez satelitního pozadí). Kreslí světovou síť 1°×1° dlaždic, umožňuje
// pan/zoom myší, kliknutím vybrat konkrétní tile a tažením ohraničit
// AOI. Reálnou mapu (raster tiles, OSM attribution) doplníme později
// v Fázi 2B "full" (MapLibre-Native nebo Qt Location).

#include <QPointF>
#include <QRectF>
#include <QWidget>

namespace xps::ui {

class TileGridView : public QWidget {
    Q_OBJECT
public:
    explicit TileGridView(QWidget* parent = nullptr);

    /// Zvýrazní jednu dlaždici (sw_lat, sw_lon) — zelené pozadí.
    void set_highlighted_tile(int lat, int lon);

    /// Programaticky nastaví AOI (v WGS84 stupních).
    void set_aoi(double west_lon, double south_lat,
                 double east_lon, double north_lat);
    void clear_aoi();

    /// Zobrazí extent GeoTIFF rasteru jako oranžový obdélník (informativní,
    /// nepřepisuje AOI). Prázdný bbox ⇒ schová.
    void set_raster_bbox(double west_lon, double south_lat,
                         double east_lon, double north_lat);
    void clear_raster_bbox();

    /// Zobrazí extent DSF souboru (sim/west..north) jako zelený overlay.
    void set_dsf_bbox(double west_lon, double south_lat,
                      double east_lon, double north_lat);
    void clear_dsf_bbox();

    /// Nastaví pohled tak, aby daný bbox (v stupních WGS84) vyplnil cca 87 %
    /// viewportu; bbox menší než ~0.0001° se ignoruje.
    void zoom_to_bbox(double west_lon, double south_lat,
                      double east_lon, double north_lat);
    void zoom_to_raster_bbox();
    void zoom_to_dsf_bbox();
    void zoom_to_aoi();

    /// Zkopíruje aktuální raster bbox do AOI (pokud existuje).
    /// Emituje `aoi_changed` aby se synchronizoval ProjectView.
    void use_raster_bbox_as_aoi();

    /// Vycentruje pohled na střed dané dlaždice (1°×1°).
    void center_on_tile(int lat, int lon);

    /// Resetuje pohled tak, aby byl vidět celý svět.
    void reset_view();

    // --- View persistence (QSettings save/restore) ---------------------
    double pixels_per_degree() const noexcept { return pixels_per_deg_; }
    QPointF view_center()       const noexcept { return center_world_; }
    void    set_view(QPointF center, double pixels_per_deg);

signals:
    /// Uživatel klikl na konkrétní dlaždici (SW roh v celých stupních).
    void tile_clicked(int lat, int lon);
    /// Uživatel dokončil AOI výběr (drag-rectangle s Shift).
    void aoi_changed(double west_lon, double south_lat,
                     double east_lon, double north_lat);
    /// Informativní kanál — MainWindow log forward.
    void log(const QString& level, const QString& msg);

protected:
    void paintEvent(QPaintEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void keyPressEvent(QKeyEvent* ev) override;

private:
    /// lon/lat (°) → widget pixel.
    QPointF world_to_screen(double lon, double lat) const;
    /// widget pixel → lon/lat (°).
    QPointF screen_to_world(QPointF p) const;

    void fit_world();  ///< výchozí zoom tak, aby byl svět celý vidět

    // --- Viewport state ------------------------------------------------
    double pixels_per_deg_ = 4.0;     ///< zoom (≥ 0.5, ≤ 50)
    QPointF center_world_  = QPointF(0.0, 0.0); ///< (lon, lat)

    // --- Selection state ----------------------------------------------
    bool   has_tile_       = false;
    int    tile_lat_       = 0;
    int    tile_lon_       = 0;

    bool   has_aoi_        = false;
    QRectF aoi_;  ///< (x = lon_west, y = lat_south, w = Δlon, h = Δlat)

    bool   has_raster_bbox_ = false;
    QRectF raster_bbox_;  ///< same convention as aoi_

    bool   has_dsf_bbox_ = false;
    QRectF dsf_bbox_;

    // --- Drag state ---------------------------------------------------
    bool      dragging_pan_ = false;
    bool      dragging_aoi_ = false;
    QPointF   drag_start_screen_;
    QPointF   drag_start_world_;  // AOI mode: world coord of drag start
    QPointF   drag_current_world_;
};

}  // namespace xps::ui
