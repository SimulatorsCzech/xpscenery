// xpscenery-ui — DSF atom inspector implementation
#include "dsf_inspector_view.hpp"

#include "xpscenery/io_dsf/dsf_writer.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <filesystem>

namespace xps::ui {

namespace {
constexpr int kRolePath  = Qt::UserRole + 1;  ///< stores hex-path index for selection

QString hex_dump(const std::vector<std::byte>& bytes, std::size_t max_bytes = 256) {
    const std::size_t n = std::min(max_bytes, bytes.size());
    QString line;
    QString out;
    for (std::size_t i = 0; i < n; ++i) {
        if (i % 16 == 0) {
            if (i) out += '\n';
            out += QStringLiteral("%1  ").arg(qulonglong(i), 8, 16, QChar('0')).toUpper();
        }
        out += QStringLiteral("%1 ")
                   .arg(quint32(std::uint8_t(bytes[i])), 2, 16, QChar('0'))
                   .toUpper();
    }
    if (bytes.size() > n) {
        out += QStringLiteral("\n… (%1 more bytes)").arg(bytes.size() - n);
    }
    return out;
}
}  // namespace

DsfInspectorView::DsfInspectorView(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    path_edit_ = new QLineEdit(this);
    path_edit_->setPlaceholderText(tr("Path to .dsf file…"));
    auto* browse = new QPushButton(tr("Browse…"), this);
    connect(browse, &QPushButton::clicked, this, &DsfInspectorView::on_browse);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(browse, 0);
    lay->addLayout(toolbar);

    summary_lbl_ = new QLabel(this);
    summary_lbl_->setTextFormat(Qt::RichText);
    lay->addWidget(summary_lbl_);

    auto* split = new QSplitter(Qt::Horizontal, this);
    tree_ = new QTreeWidget(split);
    tree_->setHeaderLabels({tr("Atom"), tr("Raw ID"), tr("Size (bytes)")});
    tree_->setRootIsDecorated(true);
    tree_->setUniformRowHeights(true);

    hex_view_ = new QPlainTextEdit(split);
    hex_view_->setReadOnly(true);
    hex_view_->setFont(QFont(QStringLiteral("Consolas"), 9));
    hex_view_->setLineWrapMode(QPlainTextEdit::NoWrap);

    split->addWidget(tree_);
    split->addWidget(hex_view_);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    lay->addWidget(split, 1);

    connect(tree_, &QTreeWidget::currentItemChanged, this,
            &DsfInspectorView::on_item_selected);
    connect(path_edit_, &QLineEdit::returnPressed, this,
            [this]() { populate(path_edit_->text()); });
}

void DsfInspectorView::on_browse() {
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Open DSF"), path_edit_->text(),
        tr("X-Plane DSF (*.dsf);;All files (*.*)"));
    if (!f.isEmpty()) open_file(f);
}

void DsfInspectorView::open_file(const QString& path) {
    path_edit_->setText(path);
    populate(path);
}

void DsfInspectorView::populate(const QString& path) {
    tree_->clear();
    hex_view_->clear();
    if (path.isEmpty()) { summary_lbl_->clear(); return; }

    const std::filesystem::path p{path.toStdWString()};
    auto r = io_dsf::read_dsf_blob(p);
    if (!r) {
        summary_lbl_->setText(tr("<b style='color:#f88'>Error:</b> %1")
                              .arg(QString::fromStdString(r.error()).toHtmlEscaped()));
        emit log(QStringLiteral("err"), QStringLiteral("dsf: %1")
                 .arg(QString::fromStdString(r.error())));
        return;
    }
    const auto& blob = *r;
    std::uint64_t total_bytes = 0;
    for (const auto& a : blob.atoms) total_bytes += a.bytes.size();

    summary_lbl_->setText(tr(
        "<b>DSF version:</b> %1 &nbsp;&nbsp;&nbsp; "
        "<b>top-level atoms:</b> %2 &nbsp;&nbsp;&nbsp; "
        "<b>payload:</b> %3 bytes")
        .arg(blob.version).arg(blob.atoms.size()).arg(total_bytes));

    for (int i = 0; i < int(blob.atoms.size()); ++i) {
        const auto& a = blob.atoms[size_t(i)];
        auto* item = new QTreeWidgetItem(tree_);
        item->setText(0, QString::fromStdString(a.tag));
        item->setText(1, QStringLiteral("0x%1").arg(a.raw_id, 8, 16, QChar('0')).toUpper());
        item->setText(2, QString::number(a.bytes.size()));
        item->setData(0, kRolePath, i);
    }
    tree_->resizeColumnToContents(0);
    tree_->resizeColumnToContents(1);
    tree_->resizeColumnToContents(2);

    // Cache blob for selection previews via dynamic property.
    // Store a shared pointer in an object property to keep the bytes alive.
    auto* holder = new QObject(this);
    holder->setObjectName(QStringLiteral("dsf_blob_holder"));
    // Remove any previous holder.
    for (auto* child : findChildren<QObject*>(QStringLiteral("dsf_blob_holder"))) {
        if (child != holder) child->deleteLater();
    }
    auto* boxed = new std::vector<io_dsf::AtomBlob>(blob.atoms);
    holder->setProperty("ptr", QVariant::fromValue(reinterpret_cast<quintptr>(boxed)));
    connect(holder, &QObject::destroyed, [boxed]() { delete boxed; });

    emit log(QStringLiteral("info"), QStringLiteral("dsf: loaded %1 (%2 atoms)")
             .arg(path).arg(blob.atoms.size()));
}

void DsfInspectorView::on_item_selected(QTreeWidgetItem* cur, QTreeWidgetItem*) {
    hex_view_->clear();
    if (!cur) return;
    const int idx = cur->data(0, kRolePath).toInt();

    QObject* holder = findChild<QObject*>(QStringLiteral("dsf_blob_holder"));
    if (!holder) return;
    auto* boxed = reinterpret_cast<std::vector<io_dsf::AtomBlob>*>(
        holder->property("ptr").value<quintptr>());
    if (!boxed || idx < 0 || idx >= int(boxed->size())) return;

    const auto& a = (*boxed)[size_t(idx)];
    hex_view_->setPlainText(hex_dump(a.bytes));
}

}  // namespace xps::ui
