#pragma once
// Project editor over xps::io_config::TileConfig.
// Loads/saves .xpsproj JSON via io_config::load/dump (the modern
// replacement for legacy RenderFarm command-list JSONs).

#include <QWidget>

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QTableWidget;
class QLabel;

namespace xps::ui {

class ProjectView : public QWidget {
    Q_OBJECT
public:
    explicit ProjectView(QWidget* parent = nullptr);
    ~ProjectView() override;

public slots:
    void open_file(const QString& path);
    /// Programatické přepsání tile lat/lon (z mapy).
    void set_tile(int lat, int lon);
    /// Programatické přepsání AOI (z mapy).
    void set_aoi(double west, double south, double east, double north);

signals:
    void log(const QString& level, const QString& msg);
    /// Emitováno kdykoli uživatel změní tile spinboxy — mapa pak zvýrazní.
    void tile_changed(int lat, int lon);
    /// Emitováno po načtení projektu, pokud AOI existuje — mapa ji zobrazí.
    void aoi_loaded(double west, double south, double east, double north);

private slots:
    void on_browse_open();
    void on_browse_save();
    void on_new_project();
    void on_layer_add();
    void on_layer_remove();
    void on_layer_up();
    void on_layer_down();
    void on_layer_browse_path();

private:
    void refresh_ui();  ///< model → widgets
    void collect();      ///< widgets → model

    QLineEdit*      path_edit_   = nullptr;
    QLabel*         summary_lbl_ = nullptr;

    // Tile
    QSpinBox*       lat_spin_    = nullptr;
    QSpinBox*       lon_spin_    = nullptr;
    QSpinBox*       schema_spin_ = nullptr;
    QLineEdit*      output_dsf_  = nullptr;

    // Meshing
    QDoubleSpinBox* max_edge_    = nullptr;
    QDoubleSpinBox* min_tri_     = nullptr;
    QDoubleSpinBox* smoothing_   = nullptr;

    // Export
    QCheckBox*      bit_ident_   = nullptr;
    QCheckBox*      gen_overlay_ = nullptr;
    QCheckBox*      compress_    = nullptr;

    // Layers
    QTableWidget*   layers_      = nullptr;

    // Hidden model (round-tripped to preserve unknown keys etc.)
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace xps::ui
