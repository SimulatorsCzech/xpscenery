// xpscenery-ui — Project (TileConfig) editor implementation
#include "project_view.hpp"

#include "xpscenery/io_config/tile_config.hpp"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
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

    // Layers group
    auto* lay_box = new QGroupBox(tr("Layers"), this);
    auto* lay_v   = new QVBoxLayout(lay_box);
    layers_ = new QTableWidget(0, 6, lay_box);
    layers_->setHorizontalHeaderLabels(
        {tr("ID"), tr("Kind"), tr("Path"), tr("SRS"), tr("Priority"), tr("Enabled")});
    layers_->horizontalHeader()->setStretchLastSection(true);
    layers_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
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
    // Layers are presently read-only in the UI; edits happen via JSON.
}

}  // namespace xps::ui
