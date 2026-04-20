#pragma once
// X-Plane OBJ8 header viewer. Displays ObjInfo from xps::io_obj::read_obj_info.

#include <QWidget>

class QLineEdit;
class QLabel;
class QFormLayout;
class QTextBrowser;

namespace xps::ui {

class ObjViewerView : public QWidget {
    Q_OBJECT
public:
    explicit ObjViewerView(QWidget* parent = nullptr);

signals:
    void log(const QString& level, const QString& msg);

public slots:
    void open_file(const QString& path);

private slots:
    void on_browse();

private:
    void populate(const QString& path);

    QLineEdit*    path_edit_ = nullptr;
    QTextBrowser* details_   = nullptr;
};

}  // namespace xps::ui
