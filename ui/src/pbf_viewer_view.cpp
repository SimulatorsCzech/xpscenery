// xpscenery-ui — OSM PBF header viewer implementation
#include "pbf_viewer_view.hpp"

#include "xpscenery/io_osm/pbf_header.hpp"

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

PbfViewerView::PbfViewerView(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    path_edit_ = new QLineEdit(this);
    path_edit_->setPlaceholderText(tr("Path to .osm.pbf (OpenStreetMap Protocolbuffer Binary Format)…"));
    auto* browse = new QPushButton(tr("Browse…"), this);
    connect(browse, &QPushButton::clicked, this, &PbfViewerView::on_browse);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(browse, 0);
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

    auto* note = new QLabel(this);
    note->setWordWrap(true);
    note->setTextFormat(Qt::RichText);
    note->setText(tr(
        "<i>Pozn.: Deep parse (bbox z OSMHeader, node/way/relation) "
        "vyžaduje zlib-dekompresi blobů — plánováno pro pozdější fázi.</i>"));
    lay->addWidget(note);

    connect(path_edit_, &QLineEdit::returnPressed, this,
            [this]() { populate(path_edit_->text()); });
}

void PbfViewerView::on_browse() {
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Open OSM PBF"), path_edit_->text(),
        tr("OpenStreetMap PBF (*.pbf *.osm.pbf);;All files (*.*)"));
    if (!f.isEmpty()) open_file(f);
}

void PbfViewerView::open_file(const QString& path) {
    path_edit_->setText(path);
    populate(path);
}

void PbfViewerView::populate(const QString& path) {
    kv_table_->setRowCount(0);
    if (path.isEmpty()) { summary_lbl_->clear(); return; }

    const std::filesystem::path p{path.toStdWString()};
    auto r = io_osm::read_pbf_header(p);
    if (!r) {
        summary_lbl_->setText(tr("<b style='color:#f88'>Error:</b> %1")
                              .arg(QString::fromStdString(r.error()).toHtmlEscaped()));
        emit log(QStringLiteral("err"), QStringLiteral("pbf: %1")
                 .arg(QString::fromStdString(r.error())));
        return;
    }
    const auto& h = *r;
    const QString blob_type = QString::fromStdString(h.first_blob_type);
    summary_lbl_->setText(tr(
        "<b>PBF</b> &nbsp; <b>file size:</b> %1 B &nbsp; "
        "<b>první BlobHeader:</b> %2 B &nbsp; "
        "<b>typ:</b> %3 &nbsp; <b>datasize:</b> %4 B")
        .arg(qulonglong(h.file_size))
        .arg(h.first_blob_header_len)
        .arg(blob_type)
        .arg(h.first_blob_datasize));

    auto add_row = [&](const QString& k, const QString& v) {
        const int r = kv_table_->rowCount();
        kv_table_->insertRow(r);
        kv_table_->setItem(r, 0, new QTableWidgetItem(k));
        kv_table_->setItem(r, 1, new QTableWidgetItem(v));
    };
    add_row(QStringLiteral("file_size_bytes"),        QString::number(qulonglong(h.file_size)));
    add_row(QStringLiteral("first_blob_header_len"),  QString::number(h.first_blob_header_len));
    add_row(QStringLiteral("first_blob_type"),        blob_type);
    add_row(QStringLiteral("first_blob_datasize"),    QString::number(h.first_blob_datasize));
    kv_table_->resizeColumnToContents(0);

    emit log(QStringLiteral("info"),
             QStringLiteral("pbf: loaded %1 — type=%2 datasize=%3")
                 .arg(path).arg(blob_type).arg(h.first_blob_datasize));
}

}  // namespace xps::ui
