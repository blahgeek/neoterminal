#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QPaintEvent>
#include <QKeyEvent>

#include <cmath>

#include "./term_ui_widget.h"
#include "./keycodes.h"


#define ASCII_STRING " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define STATIC_TEXTS_CACHE_SIZE 4096


TermUIWidget::TermUIWidget(TermUIState* state, QWidget* parent):
QWidget(parent),
font_metrics_(QFont(), this), state_(state),
static_texts_(STATIC_TEXTS_CACHE_SIZE) {
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

    qDebug() << "cell size:" << cell_size_
        << ", grid size:" << grid_size_
        << ", grid offset:" << grid_offset_;
    emit gridSizeChanged();
}

void TermUIWidget::setFont(QFont const& font) {
    font_ = font;
    font_metrics_ = QFontMetrics(font, this);

    qDebug() << "setFont" << font_;
    this->calculateGrid();
    static_texts_.clear();
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

    int text_draw_cnt = 0, text_draw_noncached_cnt = 0;

    QSize term_size = state_->size();
    for (int y = 0 ; y < term_size.height() ; y += 1) {
        for (int x = 0 ; x < term_size.width() ;) {
            auto const& cell = state_->cell_at(x, y);
            QPointF pt_lefttop(grid_offset_.x() + x * cell_size_.width(),
                               grid_offset_.y() + y * cell_size_.height());
            int affected_cols = std::max(1, cell.contiguous_cols);

            QRectF affected_rect(pt_lefttop, QSizeF(affected_cols * cell_size_.width(), cell_size_.height()));
            QRect affected_rect_bound(
                    QPoint(std::floor(affected_rect.left()), std::floor(affected_rect.top())),
                    QPoint(std::ceil(affected_rect.right()), std::ceil(affected_rect.bottom())));
            if (!redraw_region.intersects(affected_rect_bound)) {
                x += affected_cols;
                continue;
            }

            auto const& highlight = state_->highlight(cell.highlight_id);
            bool reverse_color = highlight.reverse;
            bool draw_horizontal_cursor = false;
            bool draw_vertical_cursor = false;

            // draw cursor?
            // The cursor (if valid) must be at the begin of contiguous cols
            if (QPoint(x, y) == state_->cursor()) {
                auto const& modeinfo = state_->modeinfo();
                // TODO: modeinfo.attr_id seems useless now, we just use reversed color for now
                if (modeinfo.cursor_shape == "block")
                    reverse_color = true;
                if (modeinfo.cursor_shape == "horizontal")
                    draw_horizontal_cursor = true;
                if (modeinfo.cursor_shape == "vertical")
                    draw_vertical_cursor = true;
            }

            painter.fillRect(affected_rect,
                             reverse_color ?
                             highlight.effective_foreground() :
                             highlight.effective_background());

            QPen pen(reverse_color ?
                     highlight.effective_background() :
                     highlight.effective_foreground());
            painter.setPen(pen);

            if (draw_horizontal_cursor)
                painter.drawLine(QLineF(affected_rect.bottomLeft() - QPointF(0, 1),
                                        affected_rect.bottomRight() - QPointF(0, 1)));
            if (draw_vertical_cursor)
                painter.drawLine(QLineF(affected_rect.topLeft() + QPointF(1, 0),
                                        affected_rect.bottomLeft() + QPointF(1, 0)));

            if (cell.contiguous_cols > 0) {
                QFont font = font_;

                uint32_t text_flags = 0;
                if (highlight.italic) {
                    text_flags |= (1 << 1);
                    font.setItalic(true);
                }
                if (highlight.bold) {
                    text_flags |= (1 << 2);
                    font.setBold(true);
                }

                QPair<uint32_t, QString> static_text_key(text_flags, cell.contiguous_text);
                QStaticText *static_text = static_texts_.object(static_text_key);
                if (!static_text) {
                    static_text = new QStaticText(cell.contiguous_text);
                    static_text->setPerformanceHint(QStaticText::AggressiveCaching);
                    static_text->setTextFormat(Qt::PlainText);
                    static_text->prepare(QTransform(), font);
                    static_texts_.insert(static_text_key, static_text);
                    text_draw_noncached_cnt += 1;
                }
                text_draw_cnt += 1;

                painter.setFont(font);
                // painter.drawText(pt_lefttop + QPointF(0, font_metrics_.ascent()),
                //                  cell.contiguous_text);
                painter.drawStaticText(
                        QPointF(pt_lefttop.x(),
                                pt_lefttop.y() - (static_text->size().height() - cell_size_.height()) * 4 / 5),  // align baseline for fallback font (80% of height)
                        *static_text);

                if (highlight.underline || highlight.undercurl) // TODO: curl?
                    painter.drawLine(QLineF(affected_rect.bottomLeft() - QPointF(0, 1),
                                            affected_rect.bottomRight() - QPointF(0, 1)));
                if (highlight.strikethrough)
                    painter.drawLine(QLineF(affected_rect.topLeft() + QPointF(0, affected_rect.height() / 2),
                                            affected_rect.topRight() + QPointF(0, affected_rect.height() / 2)));
            }

            x += affected_cols;
        }
    }

    // this->paintDebugGrid(event, &painter);

    auto t1 = std::chrono::steady_clock::now();
    qDebug() << "paintEvent costs" << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << "us"
        << redraw_region.boundingRect() << text_draw_noncached_cnt << text_draw_cnt;
}

void TermUIWidget::resizeEvent(QResizeEvent* event) {
    this->calculateGrid();
}

void TermUIWidget::keyPressEvent(QKeyEvent* event) {
    std::string vim_keycodes = term_keycode_translate(event);
    qDebug() << "keyPressEvent" << event->key() << event->text() << event->modifiers() << vim_keycodes.size() << vim_keycodes.c_str();
    if (!vim_keycodes.empty()) {
        emit keyPressed(std::move(vim_keycodes));
        event->setAccepted(true);
    }
}
