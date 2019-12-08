#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QPaintEvent>

#include <cmath>

#include "./term_ui_widget.h"


#define ASCII_STRING " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"


TermUIWidget::TermUIWidget(TermUIState* state, QWidget* parent):
QWidget(parent),
font_metrics_(QFont(), this), state_(state) {
    this->calculateGrid();
}

void TermUIWidget::calculateGrid() {
    double cell_height = font_metrics_.lineSpacing();
    double cell_width = font_metrics_.width(ASCII_STRING) / strlen(ASCII_STRING);

    cell_size_ = QSizeF(cell_width, cell_height);

    int grid_width = std::floor(this->width() / cell_size_.width());
    int grid_height = std::floor(this->height() / cell_size_.height());

    grid_size_ = QSize(grid_width, grid_height);
    grid_offset_ = QPointF(
            (this->width() - grid_width * cell_width) / 2,
            (this->height() - grid_height * cell_height) / 2);

    qDebug() << "cell size: " << cell_size_
        << ", grid size: " << grid_size_
        << ", grid offset: " << grid_offset_;
    emit gridSizeChanged();
}

void TermUIWidget::setFont(QFont const& font) {
    font_ = font;
    font_metrics_ = QFontMetrics(font, this);

    qDebug() << "setFont: " << font_;
    this->calculateGrid();
    this->update();
}

void TermUIWidget::redrawCells(QRegion dirty_cells) {
    QRegion dirty_pixels;
    for (auto const& rect: dirty_cells) {
        double left = grid_offset_.x() + rect.left() * cell_size_.width();
        double top = grid_offset_.y() + rect.top() * cell_size_.height();
        double right = left + rect.width() * cell_size_.width();
        double bottom = top + rect.height() * cell_size_.height();
        dirty_pixels |= QRect(QPoint(std::floor(left), std::floor(top)),
                              QPoint(std::ceil(right), std::ceil(bottom)));
    }
    this->update(dirty_pixels);
}


void TermUIWidget::paintDebugGrid(QPaintEvent* event, QPainter* painter) {
    painter->setPen(Qt::red);

    // horizontal
    for (int i = 0 ; i < grid_size_.height() + 1 ; i += 1) {
        double y = i * cell_size_.height() + grid_offset_.y();
        painter->drawLine(QLineF(grid_offset_.x(), y,
                                 grid_offset_.x() + grid_size_.width() * cell_size_.width(), y));
    }

    // vertical
    for (int i = 0 ; i < grid_size_.width() + 1 ; i += 1) {
        double x = i * cell_size_.width() + grid_offset_.x();
        painter->drawLine(QLineF(x, grid_offset_.y(),
                                 x, grid_offset_.y() + grid_size_.height() * cell_size_.height()));
    }
}

void TermUIWidget::paintEvent(QPaintEvent* event) {
    auto redraw_region = event->region();
    auto t0 = std::chrono::steady_clock::now();

    QPainter painter(this);
    painter.setFont(font_);

    // auto const& default_highlight = state_->highlight(0);
    // painter.fillRect(QRectF(QPointF(0, 0), QSizeF(this->width(), this->height())),
    //                  default_highlight.effective_background());

    QSize term_size = state_->size();
    for (int y = 0 ; y < term_size.height() ; y += 1) {
        for (int x = 0 ; x < term_size.width() ;) {
            auto const& cell = state_->cell_at(x, y);
            QPointF pt_lefttop(grid_offset_.x() + x * cell_size_.width(),
                               grid_offset_.y() + y * cell_size_.height());
            int affected_cols = std::max(1, cell.contiguous_cols);
            x += affected_cols;

            QRectF affected_rect(pt_lefttop, QSizeF(affected_cols * cell_size_.width(), cell_size_.height()));
            QRect affected_rect_bound(
                    QPoint(std::floor(affected_rect.left()), std::floor(affected_rect.top())),
                    QPoint(std::ceil(affected_rect.right()), std::ceil(affected_rect.bottom())));
            if (!redraw_region.intersects(affected_rect_bound))
                continue;

            auto const& highlight = state_->highlight(cell.highlight_id);
            painter.fillRect(affected_rect, highlight.effective_background());

            if (cell.contiguous_cols > 0) {
                QPen pen(highlight.effective_foreground());
                painter.setPen(pen);

                // TODO: bold, italics, ..
                painter.drawText(pt_lefttop + QPointF(0, font_metrics_.ascent()),
                                 cell.contiguous_text);
                // qDebug() << cell.contiguous_text;
            }
        }
    }

    // this->paintDebugGrid(event, &painter);

    auto t1 = std::chrono::steady_clock::now();
    qDebug() << "paintEvent" << redraw_region
        // << QSize(redraw_rect.width() / cell_size_.width(), redraw_rect.height() / cell_size_.height())
        << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count();
}

void TermUIWidget::resizeEvent(QResizeEvent* event) {
    this->calculateGrid();
}
