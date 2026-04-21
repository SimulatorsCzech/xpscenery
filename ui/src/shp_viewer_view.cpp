// xpscenery-ui — Shapefile header viewer implementation
#include "shp_viewer_view.hpp"

#include "xpscenery/io_vector/shp_header.hpp"
#include "tile_grid_view.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <filesystem>

namespace xps::ui {

ShpViewerView::ShpViewerView(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    path_edit_ = new QLineEdit(this);
    path_edit_->setPlaceholderText(tr("Path to .shp (ESRI Shapefile main file)…"));
    auto* browse   = new QPushButton(tr("Browse…"), this);
    auto* show_btn = new QPushButton(tr("Zobrazit v mapě"), this);
    show_btn->setToolTip(tr("Přepne na záložku Mapa a přiblíží se na bbox .shp."));
    connect(browse,   &QPushButton::clicked, this, &ShpViewerView::on_browse);
    connect(show_btn, &QPushButton::clicked, this, &ShpViewerView::show_in_map);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(browse, 0);
    toolbar->addWidget(show_btn, 0);
    lay->addLayout(toolbar);

    summary_lbl_ = new QLabel(this);
    summary_lbl_->setWordWrap(true);
    summary_lbl_->setTextFormat(Qt::RichText);
    lay->addWidget(summary_lbl_);

    kv_table_ = new QTableWidget(0, 2, this);
    kv_table_->setHorizontalHeaderLabels({tr("Klíč"), tr("Hodnota")});
    kv_table_->horizontalHeader()->setStretchLastSection(true);
    kv_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lay->addWidget(kv_table_, 1);

    mini_map_ = new TileGridView(this);
    mini_map_->setMinimumHeight(160);
    mini_map_->setMaximumHeight(220);
    mini_map_->setToolTip(tr("Náhled polohy vektorové vrstvy ve světě."));
    lay->addWidget(new QLabel(tr("Náhled polohy (overview)"), this));
    lay->addWidget(mini_map_);

    connect(path_edit_, &QLineEdit::returnPressed, this,
            [this]() { populate(path_edit_->text()); });
}

void ShpViewerView::on_browse() {
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Open Shapefile"), path_edit_->text(),
        tr("ESRI Shapefile (*.shp);;All files (*.*)"));
    if (!f.isEmpty()) open_file(f);
}

void ShpViewerView::open_file(const QString& path) {
    path_edit_->setText(path);
    populate(path);
}

void ShpViewerView::populate(const QString& path) {
    kv_table_->setRowCount(0);
    if (path.isEmpty()) { summary_lbl_->clear(); return; }

    const std::filesystem::path p{path.toStdWString()};
    auto r = io_vector::read_shp_header(p);
    if (!r) {
        summary_lbl_->setText(tr("<b style='color:#f88'>Error:</b> %1")
                              .arg(QString::fromStdString(r.error()).toHtmlEscaped()));
        emit log(QStringLiteral("err"), QStringLiteral("shp: %1")
                 .arg(QString::fromStdString(r.error())));
        return;
    }
    const auto& h = *r;
    const QString st = QString::fromStdString(
        io_vector::shape_type_name(h.shape_type_raw));

    summary_lbl_->setText(tr(
        "<b>Shapefile typ:</b> %1 &nbsp;&nbsp; "
        "<b>verze:</b> %2 &nbsp;&nbsp; "
        "<b>délka (bytes):</b> %3 &nbsp;&nbsp; "
        "<b>bbox:</b> X[%4..%5] Y[%6..%7]")
        .arg(st).arg(h.version)
        .arg(qulonglong(h.file_length_bytes))
        .arg(h.x_min, 0, 'f', 3).arg(h.x_max, 0, 'f', 3)
        .arg(h.y_min, 0, 'f', 3).arg(h.y_max, 0, 'f', 3));

    auto add_row = [&](const QString& k, const QString& v) {
        const int r = kv_table_->rowCount();
        kv_table_->insertRow(r);
        kv_table_->setItem(r, 0, new QTableWidgetItem(k));
        kv_table_->setItem(r, 1, new QTableWidgetItem(v));
    };
    add_row(QStringLiteral("file_code"),  QString::number(h.file_code));
    add_row(QStringLiteral("version"),    QString::number(h.version));
    add_row(QStringLiteral("shape_type"), QStringLiteral("%1 (%2)")
                .arg(h.shape_type_raw).arg(st));
    add_row(QStringLiteral("file_length_bytes"),
            QString::number(qulonglong(h.file_length_bytes)));
    add_row(QStringLiteral("X min/max"), QStringLiteral("%1 / %2")
                .arg(h.x_min, 0, 'g', 10).arg(h.x_max, 0, 'g', 10));
    add_row(QStringLiteral("Y min/max"), QStringLiteral("%1 / %2")
                .arg(h.y_min, 0, 'g', 10).arg(h.y_max, 0, 'g', 10));
    add_row(QStringLiteral("Z min/max"), QStringLiteral("%1 / %2")
                .arg(h.z_min, 0, 'g', 10).arg(h.z_max, 0, 'g', 10));
    add_row(QStringLiteral("M min/max"), QStringLiteral("%1 / %2")
                .arg(h.m_min, 0, 'g', 10).arg(h.m_max, 0, 'g', 10));
    kv_table_->resizeColumnToContents(0);

    emit log(QStringLiteral("info"),
             QStringLiteral("shp: loaded %1 — %2").arg(path).arg(st));

    if (h.has_valid_bbox()
        && h.x_min >= -180.0 && h.x_max <= 180.0
        && h.y_min >=  -90.0 && h.y_max <=  90.0) {
        emit shp_bbox(h.x_min, h.y_min, h.x_max, h.y_max);
        if (mini_map_) {
            // Reuse the raster bbox slot visually (orange) — je informativní
            // overlay, barvou se od DSF/AOI odlišuje dostatečně.
            mini_map_->set_raster_bbox(h.x_min, h.y_min, h.x_max, h.y_max);
            mini_map_->zoom_to_raster_bbox();
        }
        emit log(QStringLiteral("info"),
                 QStringLiteral("shp: bbox W=%1 S=%2 E=%3 N=%4")
                     .arg(h.x_min, 0, 'f', 4).arg(h.y_min, 0, 'f', 4)
                     .arg(h.x_max, 0, 'f', 4).arg(h.y_max, 0, 'f', 4));
    } else {
        if (mini_map_) mini_map_->clear_raster_bbox();
        emit log(QStringLiteral("warn"),
            tr("shp: bbox mimo WGS84 rozsah (projektovaný SRS?) — mini-mapa přeskočena"));
    }
}

}  // namespace xps::ui
