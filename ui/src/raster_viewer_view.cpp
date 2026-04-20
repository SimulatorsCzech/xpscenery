// xpscenery-ui — Raster (GeoTIFF) viewer implementation
#include "raster_viewer_view.hpp"

#include "xpscenery/io_raster/geotiff_ifd.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

#include <filesystem>

namespace xps::ui {

RasterViewerView::RasterViewerView(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    path_edit_ = new QLineEdit(this);
    path_edit_->setPlaceholderText(tr("Path to .tif/.tiff (classic TIFF or BigTIFF)…"));
    auto* browse = new QPushButton(tr("Browse…"), this);
    connect(browse, &QPushButton::clicked, this, &RasterViewerView::on_browse);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(browse, 0);
    lay->addLayout(toolbar);

    summary_lbl_ = new QLabel(this);
    summary_lbl_->setWordWrap(true);
    summary_lbl_->setTextFormat(Qt::RichText);
    lay->addWidget(summary_lbl_);

    auto* split = new QSplitter(Qt::Vertical, this);

    auto* gk_wrap = new QWidget(split);
    auto* gk_lay  = new QVBoxLayout(gk_wrap);
    gk_lay->setContentsMargins(0, 0, 0, 0);
    gk_lay->addWidget(new QLabel(tr("GeoKeyDirectory (tag 34735)"), gk_wrap));
    table_ = new QTableWidget(0, 4, gk_wrap);
    table_->setHorizontalHeaderLabels(
        {tr("Key ID"), tr("Location"), tr("Count"), tr("Value")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gk_lay->addWidget(table_);

    auto* tp_wrap = new QWidget(split);
    auto* tp_lay  = new QVBoxLayout(tp_wrap);
    tp_lay->setContentsMargins(0, 0, 0, 0);
    tp_lay->addWidget(new QLabel(tr("ModelTiepointTag (33922) — (I,J,K)→(X,Y,Z)"), tp_wrap));
    tiepoints_ = new QTableWidget(0, 6, tp_wrap);
    tiepoints_->setHorizontalHeaderLabels({"I", "J", "K", "X", "Y", "Z"});
    tiepoints_->horizontalHeader()->setStretchLastSection(true);
    tiepoints_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tp_lay->addWidget(tiepoints_);

    split->addWidget(gk_wrap);
    split->addWidget(tp_wrap);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    lay->addWidget(split, 1);

    connect(path_edit_, &QLineEdit::returnPressed, this,
            [this]() { populate(path_edit_->text()); });
}

void RasterViewerView::on_browse() {
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Open GeoTIFF"), path_edit_->text(),
        tr("GeoTIFF (*.tif *.tiff);;All files (*.*)"));
    if (!f.isEmpty()) open_file(f);
}

void RasterViewerView::open_file(const QString& path) {
    path_edit_->setText(path);
    populate(path);
}

void RasterViewerView::populate(const QString& path) {
    table_->setRowCount(0);
    tiepoints_->setRowCount(0);
    if (path.isEmpty()) { summary_lbl_->clear(); return; }

    const std::filesystem::path p{path.toStdWString()};
    auto r = io_raster::read_geotiff_ifd(p);
    if (!r) {
        summary_lbl_->setText(tr("<b style='color:#f88'>Error:</b> %1")
                              .arg(QString::fromStdString(r.error()).toHtmlEscaped()));
        emit log(QStringLiteral("err"), QStringLiteral("raster: %1")
                 .arg(QString::fromStdString(r.error())));
        return;
    }
    const auto& info = *r;

    QString s;
    s += QStringLiteral("<table cellpadding='4'>");
    auto row = [&](const QString& k, const QString& v) {
        s += QStringLiteral("<tr><td><b>%1</b></td><td>%2</td></tr>").arg(k, v);
    };
    row(tr("Format"),              info.is_bigtiff ? "BigTIFF (64-bit)" : "Classic TIFF (32-bit)");
    row(tr("Dimensions"),          QStringLiteral("%1 × %2").arg(info.width).arg(info.height));
    row(tr("Bits / sample"),       QString::number(info.bits_per_sample));
    row(tr("Samples / pixel"),     QString::number(info.samples_per_pixel));
    row(tr("Sample format"),
        info.sample_format == 1 ? "1 (uint)"
        : info.sample_format == 2 ? "2 (sint)"
        : info.sample_format == 3 ? "3 (ieee float)"
        : QString::number(info.sample_format));
    row(tr("Compression (tag 259)"), QString::number(info.compression));
    row(tr("Photometric (tag 262)"), QString::number(info.photometric));
    row(tr("Rows / strip"),          QString::number(info.rows_per_strip));
    row(tr("Strip count"),           QString::number(info.strip_count));
    row(tr("Georeferenced"),
        info.is_georeferenced() ? tr("yes") : tr("no"));
    if (info.have_pixel_scale) {
        row(tr("ModelPixelScale (33550)"),
            QStringLiteral("%1, %2, %3")
                .arg(info.pixel_scale_x, 0, 'g', 10)
                .arg(info.pixel_scale_y, 0, 'g', 10)
                .arg(info.pixel_scale_z, 0, 'g', 10));
    }
    row(tr("GeoKey revision"),
        QStringLiteral("%1.%2").arg(info.geo_key_revision).arg(info.geo_key_minor_revision));
    s += QStringLiteral("</table>");
    summary_lbl_->setText(s);

    table_->setRowCount(int(info.geo_keys.size()));
    for (int i = 0; i < int(info.geo_keys.size()); ++i) {
        const auto& k = info.geo_keys[size_t(i)];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(k.key_id)));
        table_->setItem(i, 1, new QTableWidgetItem(QString::number(k.location)));
        table_->setItem(i, 2, new QTableWidgetItem(QString::number(k.count)));
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(k.value)));
    }
    tiepoints_->setRowCount(int(info.tiepoints.size()));
    for (int i = 0; i < int(info.tiepoints.size()); ++i) {
        const auto& t = info.tiepoints[size_t(i)];
        auto cell = [](double v) {
            return new QTableWidgetItem(QString::number(v, 'g', 10));
        };
        tiepoints_->setItem(i, 0, cell(t.i));
        tiepoints_->setItem(i, 1, cell(t.j));
        tiepoints_->setItem(i, 2, cell(t.k));
        tiepoints_->setItem(i, 3, cell(t.x));
        tiepoints_->setItem(i, 4, cell(t.y));
        tiepoints_->setItem(i, 5, cell(t.z));
    }
    emit log(QStringLiteral("info"), QStringLiteral("raster: loaded %1").arg(path));
}

}  // namespace xps::ui
