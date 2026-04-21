// xpscenery-ui — TileGridView (pure QPainter world grid)
#include "tile_grid_view.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace xps::ui {

namespace {
constexpr double kMinZoom =  0.5;  // min px / °
constexpr double kMaxZoom = 50.0;  // max px / °

QColor c_bg()        { return QColor("#1e1f22"); }
QColor c_ocean()     { return QColor("#25374e"); }
QColor c_grid_min()  { return QColor("#2f333b"); }
QColor c_grid_maj()  { return QColor("#4a525d"); }
QColor c_axis()      { return QColor("#707a87"); }
QColor c_tile_hi()   { return QColor(78, 201, 176, 120); } // semi-green
QColor c_tile_hi_e() { return QColor("#4ec9b0"); }
QColor c_aoi_fill()  { return QColor(10, 108, 255, 60); }
QColor c_aoi_edge()  { return QColor("#0a6cff"); }
QColor c_text()      { return QColor("#cdd1d5"); }
}  // namespace

// -----------------------------------------------------------------------------

TileGridView::TileGridView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
    setMinimumSize(400, 300);
    setToolTip(tr("Kolečko = zoom · tažení = posun · Shift+tažení = AOI · "
                  "klik = výběr dlaždice"));
}

void TileGridView::set_highlighted_tile(int lat, int lon) {
    has_tile_ = true;
    tile_lat_ = std::clamp(lat, -89, 89);
    tile_lon_ = std::clamp(lon, -180, 179);
    update();
}

void TileGridView::set_aoi(double w, double s, double e, double n) {
    has_aoi_ = true;
    if (e < w) std::swap(e, w);
    if (n < s) std::swap(n, s);
    aoi_ = QRectF(w, s, e - w, n - s);
    update();
}

void TileGridView::clear_aoi() {
    has_aoi_ = false;
    update();
}

void TileGridView::set_raster_bbox(double w, double s, double e, double n) {
    if (e <= w || n <= s) { clear_raster_bbox(); return; }
    has_raster_bbox_ = true;
    raster_bbox_ = QRectF(w, s, e - w, n - s);
    update();
}

void TileGridView::clear_raster_bbox() {
    has_raster_bbox_ = false;
    update();
}

void TileGridView::set_dsf_bbox(double w, double s, double e, double n) {
    if (e <= w || n <= s) { clear_dsf_bbox(); return; }
    has_dsf_bbox_ = true;
    dsf_bbox_ = QRectF(w, s, e - w, n - s);
    update();
}

void TileGridView::clear_dsf_bbox() {
    has_dsf_bbox_ = false;
    update();
}

void TileGridView::zoom_to_bbox(double w, double s, double e, double n) {
    const double dw = e - w, dh = n - s;
    if (dw < 1e-4 || dh < 1e-4) return;
    constexpr double kMargin = 1.15; // 15 % border
    const double ppd_x = width()  / (dw * kMargin);
    const double ppd_y = height() / (dh * kMargin);
    double ppd = std::min(ppd_x, ppd_y);
    ppd = std::clamp(ppd, 0.5, 50.0);
    pixels_per_deg_ = ppd;
    center_world_ = QPointF((w + e) * 0.5, (s + n) * 0.5);
    update();
}

void TileGridView::zoom_to_raster_bbox() {
    if (!has_raster_bbox_) return;
    zoom_to_bbox(raster_bbox_.left(),  raster_bbox_.top(),
                 raster_bbox_.right(), raster_bbox_.top() + raster_bbox_.height());
}

void TileGridView::zoom_to_dsf_bbox() {
    if (!has_dsf_bbox_) return;
    zoom_to_bbox(dsf_bbox_.left(),  dsf_bbox_.top(),
                 dsf_bbox_.right(), dsf_bbox_.top() + dsf_bbox_.height());
}

void TileGridView::zoom_to_aoi() {
    if (!has_aoi_) return;
    zoom_to_bbox(aoi_.left(),  aoi_.top(),
                 aoi_.right(), aoi_.top() + aoi_.height());
}

void TileGridView::use_raster_bbox_as_aoi() {
    if (!has_raster_bbox_) {
        emit log(QStringLiteral("warn"),
                 tr("mapa: žádný raster bbox k převzetí do AOI"));
        return;
    }
    const double w = raster_bbox_.left();
    const double s = raster_bbox_.top();
    const double e = raster_bbox_.left() + raster_bbox_.width();
    const double n = raster_bbox_.top()  + raster_bbox_.height();
    set_aoi(w, s, e, n);
    emit aoi_changed(w, s, e, n);
    emit log(QStringLiteral("info"),
             tr("mapa: AOI převzato z raster bbox"));
}

void TileGridView::center_on_tile(int lat, int lon) {
    center_world_ = QPointF(lon + 0.5, lat + 0.5);
    // Zoom to a comfortable single-tile view if currently zoomed out.
    if (pixels_per_deg_ < 10.0) pixels_per_deg_ = 40.0;
    update();
}

void TileGridView::reset_view() { fit_world(); }

void TileGridView::set_view(QPointF center, double pixels_per_deg) {
    center_world_   = center;
    pixels_per_deg_ = std::clamp(pixels_per_deg, 0.5, 50.0);
    update();
}

// -----------------------------------------------------------------------------
// Coordinate transforms — origin lon/lat, Y axis flipped (north up).

QPointF TileGridView::world_to_screen(double lon, double lat) const {
    const QPointF c(width() * 0.5, height() * 0.5);
    const double x = c.x() + (lon - center_world_.x()) * pixels_per_deg_;
    const double y = c.y() - (lat - center_world_.y()) * pixels_per_deg_;
    return {x, y};
}

QPointF TileGridView::screen_to_world(QPointF p) const {
    const QPointF c(width() * 0.5, height() * 0.5);
    const double lon = center_world_.x() + (p.x() - c.x()) / pixels_per_deg_;
    const double lat = center_world_.y() - (p.y() - c.y()) / pixels_per_deg_;
    return {lon, lat};
}

void TileGridView::fit_world() {
    const double need_x = 360.0;
    const double need_y = 180.0;
    const double zx = width()  / need_x;
    const double zy = height() / need_y;
    pixels_per_deg_ = std::clamp(std::min(zx, zy) * 0.95, kMinZoom, kMaxZoom);
    center_world_   = QPointF(0.0, 0.0);
    update();
}

// -----------------------------------------------------------------------------

void TileGridView::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    if (pixels_per_deg_ <= kMinZoom) fit_world();
}

void TileGridView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), c_bg());

    // World bounds visible in the current viewport:
    const QPointF tl = screen_to_world({0.0, 0.0});
    const QPointF br = screen_to_world({double(width()), double(height())});
    const double lon_min = std::max(-180.0, tl.x());
    const double lon_max = std::min( 180.0, br.x());
    const double lat_min = std::max( -90.0, br.y());
    const double lat_max = std::min(  90.0, tl.y());

    // Ocean rectangle for the entire globe (so gaps look right).
    const QPointF pTL = world_to_screen(-180.0,  90.0);
    const QPointF pBR = world_to_screen( 180.0, -90.0);
    p.fillRect(QRectF(pTL, pBR), c_ocean());

    // --- 1° minor grid ---------------------------------------------------
    if (pixels_per_deg_ >= 3.0) {
        p.setPen(QPen(c_grid_min(), 0.5));
        const int step = 1;
        for (int lon = int(std::floor(lon_min));
                 lon <= int(std::ceil(lon_max)); lon += step) {
            const double x = world_to_screen(lon, 0).x();
            p.drawLine(QPointF(x, 0), QPointF(x, height()));
        }
        for (int lat = int(std::floor(lat_min));
                 lat <= int(std::ceil(lat_max)); lat += step) {
            const double y = world_to_screen(0, lat).y();
            p.drawLine(QPointF(0, y), QPointF(width(), y));
        }
    }

    // --- 10° major grid --------------------------------------------------
    p.setPen(QPen(c_grid_maj(), 1.0));
    for (int lon = -180; lon <= 180; lon += 10) {
        const double x = world_to_screen(lon, 0).x();
        if (x < -10 || x > width() + 10) continue;
        p.drawLine(QPointF(x, 0), QPointF(x, height()));
    }
    for (int lat = -90; lat <= 90; lat += 10) {
        const double y = world_to_screen(0, lat).y();
        if (y < -10 || y > height() + 10) continue;
        p.drawLine(QPointF(0, y), QPointF(width(), y));
    }

    // --- Equator + prime meridian ---------------------------------------
    p.setPen(QPen(c_axis(), 1.5));
    const double y_eq = world_to_screen(0, 0).y();
    const double x_pm = world_to_screen(0, 0).x();
    p.drawLine(QPointF(0, y_eq), QPointF(width(), y_eq));
    p.drawLine(QPointF(x_pm, 0), QPointF(x_pm, height()));

    // --- Highlighted tile ------------------------------------------------
    if (has_tile_) {
        const QPointF tl2 = world_to_screen(tile_lon_,     tile_lat_ + 1);
        const QPointF br2 = world_to_screen(tile_lon_ + 1, tile_lat_);
        const QRectF r(tl2, br2);
        p.fillRect(r, c_tile_hi());
        p.setPen(QPen(c_tile_hi_e(), 2.0));
        p.drawRect(r);
    }

    // --- Raster bbox (informative overlay from RasterViewerView) -------
    if (has_raster_bbox_) {
        const QPointF tlR = world_to_screen(raster_bbox_.left(),
                                            raster_bbox_.top() + raster_bbox_.height());
        const QPointF brR = world_to_screen(raster_bbox_.right(),
                                            raster_bbox_.top());
        const QRectF r(tlR, brR);
        p.fillRect(r, QColor(255, 140, 0, 60));
        QPen pen(QColor(255, 140, 0), 1.5);
        pen.setStyle(Qt::DotLine);
        p.setPen(pen);
        p.drawRect(r);
    }

    // --- DSF bbox (sim/west..north from HEAD/PROP) ---------------------
    if (has_dsf_bbox_) {
        const QPointF tlD = world_to_screen(dsf_bbox_.left(),
                                            dsf_bbox_.top() + dsf_bbox_.height());
        const QPointF brD = world_to_screen(dsf_bbox_.right(),
                                            dsf_bbox_.top());
        const QRectF r(tlD, brD);
        p.fillRect(r, QColor(46, 204, 64, 50));
        QPen pen(QColor(46, 204, 64), 1.5);
        pen.setStyle(Qt::DashDotLine);
        p.setPen(pen);
        p.drawRect(r);
    }

    // --- AOI -------------------------------------------------------------
    if (has_aoi_ || dragging_aoi_) {
        QRectF world = has_aoi_ ? aoi_ : QRectF{};
        if (dragging_aoi_) {
            const double w = drag_start_world_.x();
            const double s = drag_start_world_.y();
            const double e = drag_current_world_.x();
            const double n = drag_current_world_.y();
            world = QRectF(std::min(w, e), std::min(s, n),
                           std::abs(e - w), std::abs(n - s));
        }
        const QPointF tlA = world_to_screen(world.left(),  world.top() + world.height());
        const QPointF brA = world_to_screen(world.right(), world.top());
        const QRectF r(tlA, brA);
        p.fillRect(r, c_aoi_fill());
        QPen pen(c_aoi_edge(), 1.5);
        pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.drawRect(r);
    }

    // --- Tile name labels (only when zoomed in enough) ------------------
    if (pixels_per_deg_ >= 20.0) {
        p.setPen(c_text());
        p.setFont(QFont(QStringLiteral("Segoe UI"), 7));
        for (int lon = int(std::floor(lon_min));
                 lon <  int(std::ceil(lon_max)); ++lon) {
            for (int lat = int(std::floor(lat_min));
                     lat <  int(std::ceil(lat_max)); ++lat) {
                const QPointF c2 = world_to_screen(lon + 0.5, lat + 0.5);
                // Canonical XP tile name: "+50+015" (sign, 2-digit lat, sign, 3-digit lon).
                const QString name = QStringLiteral("%1%2%3%4")
                    .arg(lat >= 0 ? QChar('+') : QChar('-'))
                    .arg(std::abs(lat), 2, 10, QChar('0'))
                    .arg(lon >= 0 ? QChar('+') : QChar('-'))
                    .arg(std::abs(lon), 3, 10, QChar('0'));
                p.drawText(QPointF(c2.x() - 18, c2.y() + 3), name);
            }
        }
    }

    // --- Axis labels (10° grid) ----------------------------------------
    p.setPen(c_text());
    p.setFont(QFont(QStringLiteral("Segoe UI"), 8));
    for (int lon = -180; lon <= 180; lon += 30) {
        const double x = world_to_screen(lon, 0).x();
        if (x < 20 || x > width() - 20) continue;
        p.drawText(QPointF(x + 2, height() - 4),
                   QStringLiteral("%1°").arg(lon));
    }
    for (int lat = -90; lat <= 90; lat += 30) {
        const double y = world_to_screen(0, lat).y();
        if (y < 14 || y > height() - 4) continue;
        p.drawText(QPointF(4, y - 2), QStringLiteral("%1°").arg(lat));
    }

    // Zoom indicator in the top-right corner.
    p.setPen(c_text());
    p.drawText(QPointF(width() - 130, 14),
               QStringLiteral("zoom: %1 px/°")
                   .arg(pixels_per_deg_, 0, 'f', 2));
}

// -----------------------------------------------------------------------------

void TileGridView::wheelEvent(QWheelEvent* ev) {
    const double ticks = ev->angleDelta().y() / 120.0;
    if (ticks == 0.0) return;
    const QPointF anchor_world = screen_to_world(ev->position());
    const double factor = std::pow(1.2, ticks);
    pixels_per_deg_ = std::clamp(pixels_per_deg_ * factor, kMinZoom, kMaxZoom);
    // Keep the world point under the cursor stable.
    const QPointF new_anchor = world_to_screen(anchor_world.x(), anchor_world.y());
    const QPointF delta_world{
        (new_anchor.x() - ev->position().x()) / pixels_per_deg_,
        -(new_anchor.y() - ev->position().y()) / pixels_per_deg_};
    center_world_ += delta_world;
    update();
    ev->accept();
}

void TileGridView::mousePressEvent(QMouseEvent* ev) {
    drag_start_screen_ = ev->position();
    drag_start_world_  = screen_to_world(ev->position());
    drag_current_world_ = drag_start_world_;
    if (ev->button() == Qt::LeftButton) {
        if (ev->modifiers() & Qt::ShiftModifier) {
            dragging_aoi_ = true;
        } else {
            dragging_pan_ = true;
            setCursor(Qt::ClosedHandCursor);
        }
    }
}

void TileGridView::mouseMoveEvent(QMouseEvent* ev) {
    if (dragging_pan_) {
        const QPointF now_world = screen_to_world(ev->position());
        const QPointF delta = drag_start_world_ - now_world;
        center_world_ += delta;
        update();
    } else if (dragging_aoi_) {
        drag_current_world_ = screen_to_world(ev->position());
        update();
    }

    // Show lat/lon readout as tooltip at the cursor.
    const QPointF w = screen_to_world(ev->position());
    QToolTip::showText(ev->globalPosition().toPoint(),
        QStringLiteral("lat %1°  lon %2°")
            .arg(w.y(), 0, 'f', 3).arg(w.x(), 0, 'f', 3),
        this);
}

void TileGridView::mouseReleaseEvent(QMouseEvent* ev) {
    if (dragging_pan_ && ev->button() == Qt::LeftButton) {
        dragging_pan_ = false;
        setCursor(Qt::ArrowCursor);

        // If the user hardly moved, treat as a click → tile select.
        const QPointF delta = ev->position() - drag_start_screen_;
        if (std::hypot(delta.x(), delta.y()) < 3.0) {
            const QPointF w = screen_to_world(ev->position());
            const int lat = int(std::floor(w.y()));
            const int lon = int(std::floor(w.x()));
            if (lat >= -89 && lat <= 89 && lon >= -180 && lon <= 179) {
                set_highlighted_tile(lat, lon);
                emit tile_clicked(lat, lon);
                emit log(QStringLiteral("info"),
                    tr("mapa: vybrána dlaždice %1%2 %3%4")
                        .arg(lat >= 0 ? '+' : '-')
                        .arg(std::abs(lat), 2, 10, QChar('0'))
                        .arg(lon >= 0 ? '+' : '-')
                        .arg(std::abs(lon), 3, 10, QChar('0')));
            }
        }
    } else if (dragging_aoi_ && ev->button() == Qt::LeftButton) {
        dragging_aoi_ = false;
        const double w = drag_start_world_.x();
        const double s = drag_start_world_.y();
        const double e = drag_current_world_.x();
        const double n = drag_current_world_.y();
        if (std::abs(e - w) > 0.01 && std::abs(n - s) > 0.01) {
            set_aoi(std::min(w, e), std::min(s, n),
                    std::max(w, e), std::max(s, n));
            emit aoi_changed(aoi_.left(), aoi_.top(),
                             aoi_.left() + aoi_.width(),
                             aoi_.top()  + aoi_.height());
            emit log(QStringLiteral("info"),
                tr("mapa: AOI [W=%1 S=%2 E=%3 N=%4]")
                    .arg(aoi_.left(),  0, 'f', 3)
                    .arg(aoi_.top(),   0, 'f', 3)
                    .arg(aoi_.left() + aoi_.width(),  0, 'f', 3)
                    .arg(aoi_.top()  + aoi_.height(), 0, 'f', 3));
        }
    }
}

void TileGridView::keyPressEvent(QKeyEvent* ev) {
    // Pan in whole-pixel steps so it feels snappy.  Shift = 10° step.
    const double step_deg = (ev->modifiers() & Qt::ShiftModifier) ? 10.0 : 1.0;
    switch (ev->key()) {
        case Qt::Key_Left:  center_world_.rx() -= step_deg; update(); break;
        case Qt::Key_Right: center_world_.rx() += step_deg; update(); break;
        case Qt::Key_Up:    center_world_.ry() += step_deg; update(); break;
        case Qt::Key_Down:  center_world_.ry() -= step_deg; update(); break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            pixels_per_deg_ = std::min(kMaxZoom, pixels_per_deg_ * 1.25); update(); break;
        case Qt::Key_Minus:
            pixels_per_deg_ = std::max(kMinZoom, pixels_per_deg_ / 1.25); update(); break;
        case Qt::Key_Home: reset_view(); break;
        case Qt::Key_Escape: clear_aoi(); break;
        default: QWidget::keyPressEvent(ev); return;
    }
    ev->accept();
}

}  // namespace xps::ui
