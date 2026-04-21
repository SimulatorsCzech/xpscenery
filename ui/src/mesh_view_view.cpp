// xpscenery/ui/src/mesh_view_view.cpp
#include "mesh_view_view.hpp"

#include "xpscenery/mesh_core/delaunay.hpp"
#include "xpscenery/mesh_core/mesh_io.hpp"
#include "xpscenery/mesh_core/point.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>

#include <filesystem>
#include <span>
#include <vector>

namespace xps::ui
{

    // ---- MeshCanvas ----------------------------------------------------------
    class MeshCanvas : public QWidget
    {
        Q_OBJECT
    public:
        explicit MeshCanvas(QWidget *parent = nullptr) : QWidget(parent)
        {
            setMinimumSize(400, 300);
            setMouseTracking(true);
            setAutoFillBackground(true);
            QPalette p = palette();
            p.setColor(QPalette::Window, QColor(250, 250, 250));
            setPalette(p);
        }

        void clear_all()
        {
            points_.clear();
            mesh_ = mesh_core::TriangleMesh{};
            update();
        }

        void add_point(double x, double y)
        {
            points_.push_back({x, y, 0.0});
            mesh_ = mesh_core::TriangleMesh{}; // invalidace meshe
            update();
        }

        void set_mesh(mesh_core::TriangleMesh m)
        {
            mesh_ = std::move(m);
            update();
        }

        void set_show_edges(bool v)
        {
            show_edges_ = v;
            update();
        }

        [[nodiscard]] const std::vector<mesh_core::Point3> &points() const
        {
            return points_;
        }

        [[nodiscard]] const mesh_core::TriangleMesh &mesh() const { return mesh_; }

    signals:
        void point_added(int total);

    protected:
        void paintEvent(QPaintEvent *) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.fillRect(rect(), QColor(250, 250, 250));

            if (points_.empty())
            {
                p.setPen(QColor(150, 150, 150));
                p.drawText(rect(),
                           Qt::AlignCenter,
                           tr("Kliknete pro pridani bodu (nebo Nahodne body)."));
                return;
            }

            // Vrstva 1: trojúhelníky
            if (mesh_.triangle_count() > 0)
            {
                QColor fill(120, 180, 220, 80);
                QPen edge(QColor(40, 80, 140), 1.0);
                p.setBrush(fill);
                p.setPen(show_edges_ ? edge : Qt::NoPen);
                for (const auto &t : mesh_.triangles())
                {
                    const auto &v0 = mesh_.vertices()[t.v0];
                    const auto &v1 = mesh_.vertices()[t.v1];
                    const auto &v2 = mesh_.vertices()[t.v2];
                    QPolygonF poly;
                    poly << world_to_screen(v0.x, v0.y)
                         << world_to_screen(v1.x, v1.y)
                         << world_to_screen(v2.x, v2.y);
                    p.drawPolygon(poly);
                }
            }

            // Vrstva 2: body
            p.setBrush(QColor(200, 60, 60));
            p.setPen(QPen(QColor(120, 30, 30), 1.0));
            for (const auto &pt : points_)
            {
                QPointF s = world_to_screen(pt.x, pt.y);
                p.drawEllipse(s, 3.0, 3.0);
            }
        }

        void mousePressEvent(QMouseEvent *ev) override
        {
            if (ev->button() != Qt::LeftButton)
                return;
            auto [x, y] = screen_to_world(ev->position());
            points_.push_back({x, y, 0.0});
            mesh_ = mesh_core::TriangleMesh{};
            emit point_added(static_cast<int>(points_.size()));
            update();
        }

    private:
        // Jednoduchá fit-all transformace: pokud je N<2, použijeme jednotkový
        // viewport [0,1]×[0,1]; jinak bbox bodů s 5% rezervou.
        struct Viewport
        {
            double x0, y0, x1, y1;
        };

        [[nodiscard]] Viewport vp() const
        {
            if (points_.size() < 2)
                return {0.0, 0.0, 1.0, 1.0};
            double xmin = points_[0].x, xmax = xmin;
            double ymin = points_[0].y, ymax = ymin;
            for (const auto &p : points_)
            {
                xmin = std::min(xmin, p.x);
                xmax = std::max(xmax, p.x);
                ymin = std::min(ymin, p.y);
                ymax = std::max(ymax, p.y);
            }
            double dx = xmax - xmin;
            double dy = ymax - ymin;
            if (dx <= 0) dx = 1;
            if (dy <= 0) dy = 1;
            double pad_x = dx * 0.05;
            double pad_y = dy * 0.05;
            return {xmin - pad_x, ymin - pad_y, xmax + pad_x, ymax + pad_y};
        }

        [[nodiscard]] QPointF world_to_screen(double x, double y) const
        {
            Viewport v = vp();
            double w = width();
            double h = height();
            double sx = (x - v.x0) / (v.x1 - v.x0) * w;
            double sy = h - ((y - v.y0) / (v.y1 - v.y0) * h); // Y-flip
            return QPointF(sx, sy);
        }

        [[nodiscard]] std::pair<double, double> screen_to_world(QPointF s) const
        {
            Viewport v = vp();
            double w = width();
            double h = height();
            double x = v.x0 + (s.x() / w) * (v.x1 - v.x0);
            double y = v.y0 + ((h - s.y()) / h) * (v.y1 - v.y0);
            return {x, y};
        }

        std::vector<mesh_core::Point3> points_;
        mesh_core::TriangleMesh mesh_;
        bool show_edges_{true};
    };

    // ---- MeshViewerView ------------------------------------------------------
    MeshViewerView::MeshViewerView(QWidget *parent) : QWidget(parent)
    {
        auto *root = new QVBoxLayout(this);

        auto *bar = new QHBoxLayout();
        btn_tri_ = new QPushButton(tr("Triangulovat"));
        btn_clear_ = new QPushButton(tr("Vymazat"));
        btn_export_ = new QPushButton(tr("Export…"));
        btn_random_ = new QPushButton(tr("Náhodné body (50)"));
        format_cb_ = new QComboBox();
        format_cb_->addItems({"OBJ", "PLY"});
        show_edges_cb_ = new QCheckBox(tr("Hrany"));
        show_edges_cb_->setChecked(true);

        bar->addWidget(btn_tri_);
        bar->addWidget(btn_random_);
        bar->addWidget(btn_clear_);
        bar->addWidget(show_edges_cb_);
        bar->addStretch();
        bar->addWidget(new QLabel(tr("Formát:")));
        bar->addWidget(format_cb_);
        bar->addWidget(btn_export_);
        root->addLayout(bar);

        canvas_ = new MeshCanvas(this);
        root->addWidget(canvas_, 1);

        status_lbl_ = new QLabel(tr("Body: 0 · Trojúhelníky: 0"));
        root->addWidget(status_lbl_);

        connect(btn_tri_, &QPushButton::clicked, this,
                &MeshViewerView::on_triangulate);
        connect(btn_clear_, &QPushButton::clicked, this,
                &MeshViewerView::on_clear);
        connect(btn_export_, &QPushButton::clicked, this,
                &MeshViewerView::on_export);
        connect(btn_random_, &QPushButton::clicked, this,
                &MeshViewerView::on_random_fill);
        connect(show_edges_cb_, &QCheckBox::toggled, canvas_,
                &MeshCanvas::set_show_edges);
        connect(canvas_, &MeshCanvas::point_added, this,
                [this](int total)
                {
                    status_lbl_->setText(
                        tr("Body: %1 · Trojúhelníky: 0").arg(total));
                });
    }

    MeshViewerView::~MeshViewerView() = default;

    void MeshViewerView::on_triangulate()
    {
        const auto &pts = canvas_->points();
        if (pts.size() < 3)
        {
            emit log(tr("[mesh] potřebuji aspoň 3 body"));
            return;
        }
        auto r = mesh_core::delaunay_triangulate_2d(
            std::span<const mesh_core::Point3>{pts});
        if (!r)
        {
            emit log(tr("[mesh] triangulace selhala: %1")
                         .arg(QString::fromStdString(r.error())));
            return;
        }
        const std::size_t nt = r->triangle_count();
        const std::size_t nv = r->vertex_count();
        canvas_->set_mesh(std::move(*r));
        status_lbl_->setText(tr("Body: %1 · Trojúhelníky: %2")
                                 .arg(static_cast<int>(nv))
                                 .arg(static_cast<int>(nt)));
        emit log(tr("[mesh] Delaunay OK: %1 vrcholů, %2 trojúhelníků")
                     .arg(static_cast<int>(nv))
                     .arg(static_cast<int>(nt)));
    }

    void MeshViewerView::on_clear()
    {
        canvas_->clear_all();
        status_lbl_->setText(tr("Body: 0 · Trojúhelníky: 0"));
        emit log(tr("[mesh] plátno vyčištěno"));
    }

    void MeshViewerView::on_export()
    {
        if (canvas_->mesh().triangle_count() == 0)
        {
            emit log(tr("[mesh] nejdřív triangulujte"));
            return;
        }
        const QString fmt = format_cb_->currentText();
        const QString filter = (fmt == "OBJ") ? tr("Wavefront OBJ (*.obj)")
                                              : tr("Stanford PLY (*.ply)");
        const QString suggested =
            (fmt == "OBJ") ? "mesh.obj" : "mesh.ply";
        QString path = QFileDialog::getSaveFileName(
            this, tr("Exportovat mesh"), suggested, filter);
        if (path.isEmpty())
            return;

        std::filesystem::path fs_path(path.toStdString());
        auto res = (fmt == "OBJ")
                       ? mesh_core::write_obj(fs_path, canvas_->mesh())
                       : mesh_core::write_ply_ascii(fs_path, canvas_->mesh());
        if (!res)
        {
            emit log(tr("[mesh] export selhal: %1")
                         .arg(QString::fromStdString(res.error())));
            return;
        }
        emit log(tr("[mesh] zapsáno: %1").arg(path));
    }

    void MeshViewerView::on_random_fill()
    {
        canvas_->clear_all();
        auto *rng = QRandomGenerator::global();
        for (int i = 0; i < 50; ++i)
        {
            double x = rng->generateDouble();
            double y = rng->generateDouble();
            canvas_->add_point(x, y);
        }
        status_lbl_->setText(tr("Body: 50 · Trojúhelníky: 0"));
        emit log(tr("[mesh] přidáno 50 náhodných bodů"));
    }

} // namespace xps::ui

#include "mesh_view_view.moc"
