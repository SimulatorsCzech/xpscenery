// xpscenery-ui — OBJ viewer implementation
#include "obj_viewer_view.hpp"

#include "xpscenery/io_obj/obj_info.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <filesystem>

namespace xps::ui {

static QString platform_label(io_obj::Platform p) {
    using P = io_obj::Platform;
    switch (p) {
        case P::ibm:   return QStringLiteral("IBM (little-endian)");
        case P::apple: return QStringLiteral("Apple (historical)");
        default:       return QStringLiteral("unknown");
    }
}

ObjViewerView::ObjViewerView(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);

    auto* toolbar = new QHBoxLayout();
    path_edit_ = new QLineEdit(this);
    path_edit_->setPlaceholderText(tr("Path to .obj file…"));
    auto* browse = new QPushButton(tr("Browse…"), this);
    connect(browse, &QPushButton::clicked, this, &ObjViewerView::on_browse);
    toolbar->addWidget(path_edit_, 1);
    toolbar->addWidget(browse, 0);
    lay->addLayout(toolbar);

    details_ = new QTextBrowser(this);
    details_->setOpenExternalLinks(false);
    lay->addWidget(details_, 1);

    connect(path_edit_, &QLineEdit::returnPressed, this,
            [this]() { populate(path_edit_->text()); });
}

void ObjViewerView::on_browse() {
    const QString f = QFileDialog::getOpenFileName(
        this, tr("Open X-Plane OBJ"), path_edit_->text(),
        tr("X-Plane OBJ (*.obj);;All files (*.*)"));
    if (!f.isEmpty()) open_file(f);
}

void ObjViewerView::open_file(const QString& path) {
    path_edit_->setText(path);
    populate(path);
}

void ObjViewerView::populate(const QString& path) {
    if (path.isEmpty()) {
        details_->clear();
        return;
    }
    const std::filesystem::path p{path.toStdWString()};
    auto r = io_obj::read_obj_info(p);
    if (!r) {
        details_->setPlainText(tr("Error: %1").arg(QString::fromStdString(r.error())));
        emit log(QStringLiteral("err"), QStringLiteral("obj: %1")
                 .arg(QString::fromStdString(r.error())));
        return;
    }
    const auto& info = *r;
    QString html;
    html += QStringLiteral("<h2>OBJ8 header</h2>");
    html += QStringLiteral("<table cellpadding='4' cellspacing='0'>");
    auto row = [&](const QString& k, const QString& v) {
        html += QStringLiteral("<tr><td><b>%1</b></td><td>%2</td></tr>")
                    .arg(k, v.toHtmlEscaped());
    };
    row(tr("Platform"),       platform_label(info.platform));
    row(tr("Version"),        QString::number(info.version));
    row(tr("Texture"),        QString::fromStdString(info.texture));
    row(tr("Texture _LIT"),   QString::fromStdString(info.texture_lit));
    row(tr("Texture normal"), QString::fromStdString(info.texture_normal));
    row(tr("POINT_COUNTS vt / vline / vlight / idx"),
        QStringLiteral("%1 / %2 / %3 / %4")
            .arg(info.vt_count).arg(info.vline_count)
            .arg(info.vlight_count).arg(info.idx_count));
    row(tr("TRIS / LINES / LIGHTS commands"),
        QStringLiteral("%1 / %2 / %3")
            .arg(info.tris_commands).arg(info.lines_commands)
            .arg(info.lights_commands));
    row(tr("ANIM_begin markers"), QString::number(info.anim_begin));
    html += QStringLiteral("</table>");
    details_->setHtml(html);
    emit log(QStringLiteral("info"), QStringLiteral("obj: loaded %1").arg(path));
}

}  // namespace xps::ui
