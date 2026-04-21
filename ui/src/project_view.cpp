// xpscenery-ui — Project (TileConfig) editor implementation
#include "project_view.hpp"

#include "xpscenery/io_config/tile_config.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace xps::ui {

struct ProjectView::Impl {
    io_config::TileConfig cfg{};
    QString path;
};

ProjectView::~ProjectView() = default;

static QString path_to_qstring(const std::filesystem::path& p) {
    const auto u8 = p.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(u8.data()),
                             qsizetype(u8.size()));
}

ProjectView::ProjectView(QWidget* parent)
    : QWidget(parent), impl_(std::make_unique<Impl>()) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    path_edit_ = new QLineEdit(this);
    path_edit_->setPlaceholderText(tr("Path to .xpsproj JSON…"));
    path_edit_->setReadOnly(true);
    auto* new_btn  = new QPushButton(tr("New"),     this);
    auto* open_btn = new QPushButton(tr("Open…"),   this);
    auto* save_btn = new QPushButton(tr("Save…"),   this);
    connect(new_btn,  &QPushButton::clicked, this, &ProjectView::on_new_project);
    connect(open_btn, &QPushButton::clicked, this, &ProjectView::on_browse_open);
    connect(save_btn, &QPushButton::clicked, this, &ProjectView::on_browse_save);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(new_btn);
    toolbar->addWidget(open_btn);
    toolbar->addWidget(save_btn);
    lay->addLayout(toolbar);

    summary_lbl_ = new QLabel(this);
    summary_lbl_->setTextFormat(Qt::RichText);
    lay->addWidget(summary_lbl_);

    // Tile group
    auto* tile_box = new QGroupBox(tr("Tile"), this);
    auto* tile_form = new QFormLayout(tile_box);
    lat_spin_ = new QSpinBox(tile_box);
    lat_spin_->setRange(-89, 89);
    lon_spin_ = new QSpinBox(tile_box);
    lon_spin_->setRange(-180, 179);
    schema_spin_ = new QSpinBox(tile_box);
    schema_spin_->setRange(1, 99);
    output_dsf_ = new QLineEdit(tile_box);
    tile_form->addRow(tr("South-west latitude"), lat_spin_);
    tile_form->addRow(tr("South-west longitude"), lon_spin_);
    tile_form->addRow(tr("Schema version"), schema_spin_);
    tile_form->addRow(tr("Output DSF path"), output_dsf_);
    lay->addWidget(tile_box);

    connect(lat_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this](int v) { emit tile_changed(v, lon_spin_->value()); });
    connect(lon_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this](int v) { emit tile_changed(lat_spin_->value(), v); });

    // Meshing group
    auto* mesh_box = new QGroupBox(tr("Meshing"), this);
    auto* mesh_form = new QFormLayout(mesh_box);
    max_edge_ = new QDoubleSpinBox(mesh_box);
    max_edge_->setRange(1.0, 1'000'000.0);
    max_edge_->setDecimals(2);
    min_tri_ = new QDoubleSpinBox(mesh_box);
    min_tri_->setRange(0.0, 1'000'000.0);
    min_tri_->setDecimals(2);
    smoothing_ = new QDoubleSpinBox(mesh_box);
    smoothing_->setRange(0.0, 1.0);
    smoothing_->setDecimals(3);
    smoothing_->setSingleStep(0.05);
    mesh_form->addRow(tr("Max edge (m)"),           max_edge_);
    mesh_form->addRow(tr("Min triangle area (m²)"), min_tri_);
    mesh_form->addRow(tr("Smoothing factor"),       smoothing_);
    lay->addWidget(mesh_box);

    // Export group
    auto* exp_box = new QGroupBox(tr("Export"), this);
    auto* exp_form = new QFormLayout(exp_box);
    bit_ident_   = new QCheckBox(tr("Bit-identical with legacy baseline"), exp_box);
    gen_overlay_ = new QCheckBox(tr("Generate overlay DSF"), exp_box);
    compress_    = new QCheckBox(tr("Compress output (7-zip)"), exp_box);
    exp_form->addRow(bit_ident_);
    exp_form->addRow(gen_overlay_);
    exp_form->addRow(compress_);
    lay->addWidget(exp_box);

    // Layers group with CRUD toolbar
    auto* lay_box = new QGroupBox(tr("Layers"), this);
    auto* lay_v   = new QVBoxLayout(lay_box);
    auto* lay_tb  = new QHBoxLayout();
    auto* add_btn = new QPushButton(tr("+ Přidat"), lay_box);
    auto* del_btn = new QPushButton(tr("– Odebrat"), lay_box);
    auto* up_btn  = new QPushButton(tr("▲"),        lay_box);
    auto* dn_btn  = new QPushButton(tr("▼"),        lay_box);
    auto* pick_btn= new QPushButton(tr("Vybrat cestu…"), lay_box);
    connect(add_btn,  &QPushButton::clicked, this, &ProjectView::on_layer_add);
    connect(del_btn,  &QPushButton::clicked, this, &ProjectView::on_layer_remove);
    connect(up_btn,   &QPushButton::clicked, this, &ProjectView::on_layer_up);
    connect(dn_btn,   &QPushButton::clicked, this, &ProjectView::on_layer_down);
    connect(pick_btn, &QPushButton::clicked, this, &ProjectView::on_layer_browse_path);
    lay_tb->addWidget(add_btn);
    lay_tb->addWidget(del_btn);
    lay_tb->addWidget(up_btn);
    lay_tb->addWidget(dn_btn);
    lay_tb->addWidget(pick_btn);
    lay_tb->addStretch(1);
    lay_v->addLayout(lay_tb);
    layers_ = new QTableWidget(0, 6, lay_box);
    layers_->setHorizontalHeaderLabels(
        {tr("ID"), tr("Kind"), tr("Path"), tr("SRS"), tr("Priority"), tr("Enabled")});
    layers_->horizontalHeader()->setStretchLastSection(false);
    layers_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    layers_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layers_->setEditTriggers(QAbstractItemView::DoubleClicked
                             | QAbstractItemView::SelectedClicked
                             | QAbstractItemView::EditKeyPressed);
    lay_v->addWidget(layers_);
    lay->addWidget(lay_box, 1);

    refresh_ui();
}

void ProjectView::on_new_project() {
    impl_->cfg = io_config::TileConfig{};
    impl_->path.clear();
    path_edit_->clear();
    refresh_ui();
    emit log(QStringLiteral("info"), tr("project: new project"));
}

void ProjectView::set_tile(int lat, int lon) {
    // Blokovat signály jinak bychom nekonečně cyklili s mapou.
    QSignalBlocker bla(lat_spin_);
    QSignalBlocker blo(lon_spin_);
    lat_spin_->setValue(std::clamp(lat, -89, 89));
    lon_spin_->setValue(std::clamp(lon, -180, 179));
    if (auto tc = core_types::TileCoord::from_lat_lon(lat_spin_->value(),
                                                       lon_spin_->value())) {
        impl_->cfg.tile = *tc;
    }
    summary_lbl_->setText(
        tr("<b>Tile:</b> %1, &nbsp;<b>layers:</b> %2")
            .arg(QString::fromStdString(impl_->cfg.tile.canonical_name()))
            .arg(impl_->cfg.layers.size()));
}

void ProjectView::set_aoi(double w, double s, double e, double n) {
    const auto sw = xps::core_types::LatLon::from_lat_lon(s, w);
    const auto ne = xps::core_types::LatLon::from_lat_lon(n, e);
    impl_->cfg.aoi = xps::core_types::BoundingBox::from_corners(sw, ne);
    emit log(QStringLiteral("info"),
        tr("project: AOI nastaveno [W=%1 S=%2 E=%3 N=%4]")
            .arg(w, 0, 'f', 3).arg(s, 0, 'f', 3)
            .arg(e, 0, 'f', 3).arg(n, 0, 'f', 3));
}

void ProjectView::on_browse_open() {
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Open xpscenery project"), path_edit_->text(),
        tr("xpscenery project (*.xpsproj *.json);;All files (*.*)"));
    if (!f.isEmpty()) open_file(f);
}

void ProjectView::open_file(const QString& path) {
    const std::filesystem::path p{path.toStdWString()};
    auto r = io_config::load(p);
    if (!r) {
        QMessageBox::warning(this, tr("Project load failed"),
                             QString::fromStdString(r.error()));
        emit log(QStringLiteral("err"), QStringLiteral("project: %1")
                 .arg(QString::fromStdString(r.error())));
        return;
    }
    impl_->cfg  = *std::move(r);
    impl_->path = path;
    path_edit_->setText(path);
    refresh_ui();
    if (impl_->cfg.aoi.has_value() && !impl_->cfg.aoi->is_empty()) {
        const auto& b = *impl_->cfg.aoi;
        emit aoi_loaded(b.sw().lon(), b.sw().lat(),
                        b.ne().lon(), b.ne().lat());
    }
    emit log(QStringLiteral("info"), QStringLiteral("project: loaded %1").arg(path));
}

void ProjectView::on_browse_save() {
    QString f = QFileDialog::getSaveFileName(
        this, tr("Save xpscenery project"),
        impl_->path.isEmpty() ? QStringLiteral("untitled.xpsproj") : impl_->path,
        tr("xpscenery project (*.xpsproj);;JSON (*.json);;All files (*.*)"));
    if (f.isEmpty()) return;
    if (!f.contains('.')) f += QStringLiteral(".xpsproj");

    collect();
    const std::string text = io_config::dump(impl_->cfg);
    std::ofstream os(std::filesystem::path{f.toStdWString()}, std::ios::binary);
    if (!os) {
        QMessageBox::warning(this, tr("Save failed"),
                             tr("Cannot open file for writing: %1").arg(f));
        return;
    }
    os.write(text.data(), std::streamsize(text.size()));
    impl_->path = f;
    path_edit_->setText(f);
    emit log(QStringLiteral("info"), QStringLiteral("project: saved %1").arg(f));
}

void ProjectView::refresh_ui() {
    const auto& c = impl_->cfg;
    lat_spin_->setValue(c.tile.lat());
    lon_spin_->setValue(c.tile.lon());
    schema_spin_->setValue(c.schema_version);
    output_dsf_->setText(path_to_qstring(c.output_dsf));

    max_edge_->setValue(c.meshing.max_edge_m);
    min_tri_->setValue(c.meshing.min_triangle_area_m2);
    smoothing_->setValue(c.meshing.smoothing_factor);

    bit_ident_->setChecked(c.xp_export.bit_identical_baseline);
    gen_overlay_->setChecked(c.xp_export.generate_overlay);
    compress_->setChecked(c.xp_export.compress);

    layers_->setRowCount(int(c.layers.size()));
    for (int i = 0; i < int(c.layers.size()); ++i) {
        const auto& L = c.layers[size_t(i)];
        layers_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(L.id)));
        layers_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(L.kind)));
        layers_->setItem(i, 2, new QTableWidgetItem(path_to_qstring(L.path)));
        layers_->setItem(i, 3, new QTableWidgetItem(
            QString::fromStdString(L.srs.value_or(""))));
        layers_->setItem(i, 4, new QTableWidgetItem(QString::number(L.priority)));
        auto* enabled = new QTableWidgetItem();
        enabled->setCheckState(L.enabled ? Qt::Checked : Qt::Unchecked);
        layers_->setItem(i, 5, enabled);
    }

    summary_lbl_->setText(
        tr("<b>Tile:</b> %1, &nbsp;<b>layers:</b> %2")
            .arg(QString::fromStdString(c.tile.canonical_name()))
            .arg(c.layers.size()));
}

void ProjectView::collect() {
    auto& c = impl_->cfg;
    c.schema_version = schema_spin_->value();

    if (auto tc = core_types::TileCoord::from_lat_lon(lat_spin_->value(),
                                                      lon_spin_->value())) {
        c.tile = *tc;
    }
    c.output_dsf = std::filesystem::path{output_dsf_->text().toStdWString()};

    c.meshing.max_edge_m           = max_edge_->value();
    c.meshing.min_triangle_area_m2 = min_tri_->value();
    c.meshing.smoothing_factor     = smoothing_->value();

    c.xp_export.bit_identical_baseline = bit_ident_->isChecked();
    c.xp_export.generate_overlay       = gen_overlay_->isChecked();
    c.xp_export.compress               = compress_->isChecked();

    // Layers: gather table rows back into typed vector.
    c.layers.clear();
    c.layers.reserve(size_t(layers_->rowCount()));
    for (int r = 0; r < layers_->rowCount(); ++r) {
        io_config::LayerSource L{};
        const auto cell = [&](int col) {
            QTableWidgetItem* it = layers_->item(r, col);
            return it ? it->text() : QString();
        };
        L.id   = cell(0).toStdString();
        L.kind = cell(1).toStdString();
        L.path = std::filesystem::path{cell(2).toStdWString()};
        const QString srs = cell(3).trimmed();
        if (!srs.isEmpty()) L.srs = srs.toStdString();
        L.priority = cell(4).toInt();
        if (QTableWidgetItem* en = layers_->item(r, 5)) {
            L.enabled = (en->checkState() == Qt::Checked);
        }
        if (L.id.empty() && L.kind.empty() && L.path.empty()) continue; // skip blank rows
        c.layers.push_back(std::move(L));
    }
}

void ProjectView::on_layer_add() {
    const int r = layers_->rowCount();
    layers_->insertRow(r);
    layers_->setItem(r, 0, new QTableWidgetItem(tr("new_layer_%1").arg(r + 1)));
    layers_->setItem(r, 1, new QTableWidgetItem(QStringLiteral("geotiff")));
    layers_->setItem(r, 2, new QTableWidgetItem());
    layers_->setItem(r, 3, new QTableWidgetItem(QStringLiteral("EPSG:4326")));
    layers_->setItem(r, 4, new QTableWidgetItem(QStringLiteral("0")));
    auto* en = new QTableWidgetItem();
    en->setCheckState(Qt::Checked);
    layers_->setItem(r, 5, en);
    layers_->selectRow(r);
    emit log(QStringLiteral("info"), tr("layer: přidán řádek %1").arg(r + 1));
}

void ProjectView::on_layer_remove() {
    const int r = layers_->currentRow();
    if (r < 0) return;
    layers_->removeRow(r);
    emit log(QStringLiteral("info"), tr("layer: odstraněn řádek %1").arg(r + 1));
}

static void move_row(QTableWidget* t, int from, int to) {
    const int cols = t->columnCount();
    // Take items out of the source row.
    std::vector<QTableWidgetItem*> row_items;
    row_items.reserve(size_t(cols));
    for (int c = 0; c < cols; ++c) row_items.push_back(t->takeItem(from, c));
    t->removeRow(from);
    t->insertRow(to);
    for (int c = 0; c < cols; ++c) t->setItem(to, c, row_items[size_t(c)]);
    t->selectRow(to);
}

void ProjectView::on_layer_up() {
    const int r = layers_->currentRow();
    if (r <= 0) return;
    move_row(layers_, r, r - 1);
}

void ProjectView::on_layer_down() {
    const int r = layers_->currentRow();
    if (r < 0 || r + 1 >= layers_->rowCount()) return;
    move_row(layers_, r, r + 1);
}

void ProjectView::on_layer_browse_path() {
    const int r = layers_->currentRow();
    if (r < 0) {
        QMessageBox::information(this, tr("Vrstva"),
            tr("Nejprve vyberte řádek v tabulce."));
        return;
    }
    QTableWidgetItem* it = layers_->item(r, 2);
    const QString current = it ? it->text() : QString();
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Vybrat zdroj vrstvy"), current,
        tr("Všechny podporované (*.tif *.tiff *.shp *.osm.pbf *.dsf);;"
           "GeoTIFF (*.tif *.tiff);;Shapefile (*.shp);;OSM PBF (*.osm.pbf);;"
           "DSF (*.dsf);;Všechny soubory (*.*)"));
    if (f.isEmpty()) return;
    if (!it) { it = new QTableWidgetItem(); layers_->setItem(r, 2, it); }
    it->setText(f);
    // Auto-fill kind column if user hasn't set it.
    QTableWidgetItem* kind = layers_->item(r, 1);
    if (kind && kind->text().isEmpty()) {
        const QString sfx = QFileInfo(f).suffix().toLower();
        if (sfx == QLatin1String("tif") || sfx == QLatin1String("tiff")) kind->setText(QStringLiteral("geotiff"));
        else if (sfx == QLatin1String("shp")) kind->setText(QStringLiteral("shapefile"));
        else if (sfx == QLatin1String("pbf")) kind->setText(QStringLiteral("osm_pbf"));
        else if (sfx == QLatin1String("dsf")) kind->setText(QStringLiteral("dsf"));
    }
}

}  // namespace xps::ui
