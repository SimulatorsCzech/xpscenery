// xpscenery/ui/src/mesh_view_view.hpp
#pragma once

#include "xpscenery/mesh_core/triangle_mesh.hpp"

#include <QWidget>

#include <memory>
#include <vector>

class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;

namespace xps::ui
{

    class MeshCanvas;

    /// Jednoduchý viewer/editor 2D bodového mraku s Delaunay triangulací.
    /// Levý klik přidá bod, "Triangulovat" spustí mesh_core, "Export…" zapíše
    /// OBJ/PLY přes mesh_core::write_obj / write_ply_ascii.
    class MeshViewerView : public QWidget
    {
        Q_OBJECT

    public:
        explicit MeshViewerView(QWidget *parent = nullptr);
        ~MeshViewerView() override;

    signals:
        void log(const QString &msg);

    private slots:
        void on_triangulate();
        void on_clear();
        void on_export();
        void on_random_fill();

    private:
        MeshCanvas *canvas_{nullptr};
        QLabel *status_lbl_{nullptr};
        QPushButton *btn_tri_{nullptr};
        QPushButton *btn_clear_{nullptr};
        QPushButton *btn_export_{nullptr};
        QPushButton *btn_random_{nullptr};
        QComboBox *format_cb_{nullptr};
        QCheckBox *show_edges_cb_{nullptr};
    };

} // namespace xps::ui
