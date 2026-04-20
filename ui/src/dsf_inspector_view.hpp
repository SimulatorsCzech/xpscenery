#pragma once
// DSF atom-level inspector. Opens a .dsf file via xps::io_dsf::read_dsf_blob
// and shows each top-level atom (HEAD/DEFN/GEOD/CMDS/DEMS …) as a tree
// with size + tag + raw_id + first bytes hex preview.

#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;
class QLineEdit;
class QLabel;

namespace xps::ui {

class DsfInspectorView : public QWidget {
    Q_OBJECT
public:
    explicit DsfInspectorView(QWidget* parent = nullptr);

signals:
    void log(const QString& level, const QString& msg);

public slots:
    void open_file(const QString& path);

private slots:
    void on_browse();
    void on_item_selected(QTreeWidgetItem* cur, QTreeWidgetItem*);

private:
    void populate(const QString& path);

    QLineEdit*      path_edit_   = nullptr;
    QLabel*         summary_lbl_ = nullptr;
    QTreeWidget*    tree_        = nullptr;
    QPlainTextEdit* hex_view_    = nullptr;
};

}  // namespace xps::ui
