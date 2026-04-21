// xpscenery-ui — DSF atom inspector implementation
#include "dsf_inspector_view.hpp"

#include "xpscenery/io_dsf/dsf_writer.hpp"
#include "xpscenery/io_dsf/dsf_strings.hpp"
#include "tile_grid_view.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <filesystem>
#include <optional>

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
    auto* show_btn = new QPushButton(tr("Zobrazit v mapě"), this);
    show_btn->setToolTip(tr("Přepne na záložku Mapa a přiblíží se na bbox DSF."));
    connect(browse, &QPushButton::clicked, this, &DsfInspectorView::on_browse);
    connect(show_btn, &QPushButton::clicked, this, &DsfInspectorView::show_in_map);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(browse, 0);
    toolbar->addWidget(show_btn, 0);
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

    // Mini-map overview pro DSF tile pokrytí.
    mini_map_ = new TileGridView(this);
    mini_map_->setMinimumHeight(160);
    mini_map_->setMaximumHeight(220);
    mini_map_->setToolTip(tr("Náhled pokrytí DSF tile. Klikněte na "
                              "\"Zobrazit v mapě\" pro interaktivní pohled."));
    lay->addWidget(new QLabel(tr("Náhled pokrytí tile"), this));
    lay->addWidget(mini_map_);

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

    // File size on disk (includes 12-byte MD5 trailer, headers, etc.)
    std::error_code ec;
    const std::uintmax_t file_sz = std::filesystem::file_size(p, ec);
    const std::uint64_t overhead = (!ec && file_sz >= total_bytes)
                                   ? (file_sz - total_bytes) : 0;

    // Collect tag set for a quick "expected atoms" check.
    QStringList tags;
    for (const auto& a : blob.atoms) tags << QString::fromStdString(a.tag);
    const QSet<QString> have(tags.begin(), tags.end());
    QStringList missing;
    for (const char* expected : {"HEAD", "DEFN", "GEOD", "CMDS"}) {
        if (!have.contains(QString::fromLatin1(expected))) missing << expected;
    }
    const QString health = missing.isEmpty()
        ? QStringLiteral("<span style='color:#4ec9b0'>✓ core atoms OK</span>")
        : QStringLiteral("<span style='color:#f48771'>✗ chybí: %1</span>")
              .arg(missing.join(QStringLiteral(", ")));

    summary_lbl_->setText(tr(
        "<b>DSF verze:</b> %1 &nbsp;&nbsp; "
        "<b>top-level atomů:</b> %2 &nbsp;&nbsp; "
        "<b>payload:</b> %3 B &nbsp;&nbsp; "
        "<b>soubor:</b> %4 B &nbsp;(overhead %5 B) &nbsp;&nbsp; %6")
        .arg(blob.version).arg(blob.atoms.size())
        .arg(total_bytes).arg(qulonglong(file_sz)).arg(overhead)
        .arg(health));

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

    // Extract sim/west|south|east|north from HEAD/PROP and emit for map.
    auto props = io_dsf::read_properties(p);
    if (props) {
        auto find = [&](const char* key) -> std::optional<double> {
            for (const auto& kv : *props) {
                if (kv.first == key) {
                    try { return std::stod(kv.second); }
                    catch (...) { return std::nullopt; }
                }
            }
            return std::nullopt;
        };
        auto w = find("sim/west");
        auto s = find("sim/south");
        auto e = find("sim/east");
        auto n = find("sim/north");
        if (w && s && e && n && *e > *w && *n > *s) {
            emit dsf_bbox(*w, *s, *e, *n);
            if (mini_map_) {
                mini_map_->set_dsf_bbox(*w, *s, *e, *n);
                mini_map_->zoom_to_dsf_bbox();
            }
            emit log(QStringLiteral("info"),
                QStringLiteral("dsf: bbox W=%1 S=%2 E=%3 N=%4")
                    .arg(*w, 0, 'f', 3).arg(*s, 0, 'f', 3)
                    .arg(*e, 0, 'f', 3).arg(*n, 0, 'f', 3));
        }
    } else {
        emit log(QStringLiteral("warn"),
            QStringLiteral("dsf: read_properties selhalo — %1")
                .arg(QString::fromStdString(props.error())));
    }
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
