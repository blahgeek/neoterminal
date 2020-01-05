#include <QDebug>
#include <QPainter>
#include <QFontDatabase>
#include <QPen>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>

#include <cmath>

#include "./nvim_ui_widget.h"
#include "./keycodes.h"


#define ASCII_STRING " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define STATIC_TEXTS_CACHE_SIZE 4096


NvimUIWidget::NvimUIWidget(QWidget* parent):
QWidget(parent),
font_metrics_(QFont(), this),
static_texts_(STATIC_TEXTS_CACHE_SIZE) {
    this->setAttribute(Qt::WA_InputMethodEnabled);
    this->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

void NvimUIWidget::calculateGrid() {
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

void NvimUIWidget::setFont(QFont const& font) {
    font_ = font;
    font_metrics_ = QFontMetrics(font, this);

    qDebug() << "setFont" << font_ << font_metrics_.ascent() << font_metrics_.height() << (font_metrics_.ascent() / font_metrics_.height());
    this->calculateGrid();
    static_texts_.clear();
    this->update();
}

void NvimUIWidget::updateState(std::shared_ptr<NvimUIState> state,
                               QRegion dirty_cells, bool defaults_updated) {
    state_ = state;

    if (defaults_updated) {
        this->update();
        return;
    }

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

void NvimUIWidget::paintDebugGrid(QPaintEvent* event, QPainter* painter) {
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

void NvimUIWidget::paintEvent(QPaintEvent* event) {
    if (!state_)
        return;

    auto redraw_region = event->region();
    auto t0 = std::chrono::steady_clock::now();

    QPainter painter(this);
    painter.setFont(font_);

    // always draw areas outside grid
    {
        auto color = state_->default_background;
        painter.fillRect(QRectF(0, 0, this->width(), grid_offset_.y()), color);
        painter.fillRect(QRectF(0, 0, grid_offset_.x(), this->height()), color);

        double right = grid_offset_.x() + grid_size_.width() * cell_size_.width();
        double bottom = grid_offset_.y() + grid_size_.height() * cell_size_.height();
        painter.fillRect(QRectF(right, 0, this->width() - right, this->height()), color);
        painter.fillRect(QRectF(0, bottom, this->width(), this->height() - bottom), color);
    }

    int text_draw_cnt = 0, text_draw_noncached_cnt = 0;

    QSize nvim_size = state_->size;
    for (int y = 0 ; y < nvim_size.height() ; y += 1) {
        for (int x = 0 ; x < nvim_size.width() ;) {
            auto const& cell = state_->cells[y][x];
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

            auto const& highlight = *cell.highlight;
            bool reverse_color = highlight.reverse;
            bool draw_horizontal_cursor = false;
            bool draw_vertical_cursor = false;

            // draw cursor?
            // The cursor (if valid) must be at the begin of contiguous cols
            if (QPoint(x, y) == state_->cursor) {
                auto const& modeinfo = state_->modeinfo;
                // TODO: modeinfo.attr_id seems useless now, we just use reversed color for now
                if (modeinfo.cursor_shape == "block")
                    reverse_color = !reverse_color;
                if (modeinfo.cursor_shape == "horizontal")
                    draw_horizontal_cursor = true;
                if (modeinfo.cursor_shape == "vertical")
                    draw_vertical_cursor = true;
            }

            auto highlight_fg = highlight.foreground.isValid() ? highlight.foreground : state_->default_foreground;
            auto highlight_bg = highlight.background.isValid() ? highlight.background : state_->default_background;

            painter.fillRect(affected_rect,
                             reverse_color ?  highlight_fg : highlight_bg);

            QPen pen(reverse_color ?  highlight_bg : highlight_fg);
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
                                pt_lefttop.y() + font_metrics_.lineSpacing() - font_metrics_.height()
                                    - (static_text->size().height() - cell_size_.height()) * font_metrics_.ascent() / font_metrics_.height()),  // align baseline for fallback font
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

    if (!im_preedit_text_.isEmpty()) {
        QPen pen(state_->default_foreground);
        auto cursor = state_->cursor;
        QPointF pt_lefttop(grid_offset_.x() + cursor.x() * cell_size_.width(),
                           grid_offset_.y() + cursor.y() * cell_size_.height());
        QFont font = font_;
        font.setUnderline(true);
        painter.setFont(font);
        painter.setPen(pen);
        painter.drawText(pt_lefttop + QPointF(0, font_metrics_.ascent()),
                         im_preedit_text_);
    }


    // this->paintDebugGrid(event, &painter);

    auto t1 = std::chrono::steady_clock::now();
    qDebug() << "paintEvent costs" << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << "us"
        << redraw_region.boundingRect() << text_draw_noncached_cnt << text_draw_cnt;
}

void NvimUIWidget::resizeEvent(QResizeEvent* event) {
    this->calculateGrid();
}

void NvimUIWidget::keyPressEvent(QKeyEvent* event) {
    std::string vim_keycodes = nvim_keycode_translate(event);
    qDebug() << "keyPressEvent" << event->key() << event->text() << event->modifiers() << vim_keycodes.size() << vim_keycodes.c_str();
    if (!vim_keycodes.empty()) {
        emit keyPressed(std::move(vim_keycodes));
        event->setAccepted(true);
    }
}

QVariant NvimUIWidget::inputMethodQuery(Qt::InputMethodQuery query) const {
    switch (query) {
        case Qt::ImEnabled:
            return true;
        case Qt::ImFont:
            return font_;
        case Qt::ImCursorRectangle:
            {
                QPoint cursor = state_ ? state_->cursor : QPoint(0, 0);
                return QRect(grid_offset_.x() + cursor.x() * cell_size_.width(),
                             grid_offset_.y() + cursor.y() * cell_size_.height(),
                             cell_size_.width(), cell_size_.height());
            }
        default:
            break;
    }

    return QVariant();
}

void NvimUIWidget::inputMethodEvent(QInputMethodEvent* event) {
    qDebug() << "inputMethodEvent"
        << event->preeditString() << event->commitString()
        << event->replacementLength() << event->replacementStart();

    QPoint cursor = state_ ? state_->cursor : QPoint(0, 0);
    im_preedit_text_ = event->preeditString();
    this->update(grid_offset_.x() + cursor.x() * cell_size_.width(),
                 grid_offset_.y() + cursor.y() * cell_size_.height(),
                 (grid_size_.width() - cursor.x()) * cell_size_.width(),
                 cell_size_.height());

    if (!event->commitString().isEmpty())
        emit keyPressed(event->commitString().toStdString());

    event->setAccepted(true);
}

namespace {
    std::string get_nvim_modifiers(int mods) {
        std::string ret;
        if (mods & Qt::ShiftModifier)
            ret += "S";
        if (mods & Qt::ControlModifier)
            ret += "C";
        if (mods & Qt::AltModifier)
            ret += "A";
        if (mods & Qt::MetaModifier)
            ret += "M";
        return ret;
    }
}

void NvimUIWidget::processMouseEvent(QMouseEvent* event) {
    MouseInputParams params;
    Qt::MouseButton event_btn = Qt::NoButton;
    switch (event->type()) {
        case QEvent::MouseButtonPress:
            params.action = "press";
            pressed_mouse_btn_ = event->button();
            event_btn = event->button();
            break;
        case QEvent::MouseButtonRelease:
            params.action = "release";
            pressed_mouse_btn_ = Qt::NoButton;
            event_btn = event->button();
            break;
        case QEvent::MouseMove:
            params.action = "drag";
            event_btn = pressed_mouse_btn_;
            break;
        default:
            return;
    }
    switch (event_btn) {
        case Qt::LeftButton:
            params.button = "left";
            break;
        case Qt::RightButton:
            params.button = "right";
            break;
        case Qt::MidButton:
            params.button = "middle";
            break;
        default:
            return;
    }

    params.modifier = get_nvim_modifiers(event->modifiers());
    params.col = (event->x() - grid_offset_.x()) / cell_size_.width();
    params.row = (event->y() - grid_offset_.y()) / cell_size_.height();

    qDebug() << "mouseEvent" << params.button.c_str() << params.action.c_str() << params.modifier.c_str() << params.col << params.row;
    emit mouseInput(params);
    event->setAccepted(true);
}

void NvimUIWidget::mouseMoveEvent(QMouseEvent* event) {
    this->processMouseEvent(event);
}
void NvimUIWidget::mousePressEvent(QMouseEvent* event) {
    this->processMouseEvent(event);
}
void NvimUIWidget::mouseReleaseEvent(QMouseEvent* event) {
    this->processMouseEvent(event);
}

void NvimUIWidget::wheelEvent(QWheelEvent* event) {
    MouseInputParams params;
    params.button = "wheel";
    params.modifier = get_nvim_modifiers(event->modifiers());
    params.col = (event->x() - grid_offset_.x()) / cell_size_.width();
    params.row = (event->y() - grid_offset_.y()) / cell_size_.height();

    auto delta = event->angleDelta();
    if (std::abs(delta.x()) > std::abs(delta.y())) {  // horizontal
        params.action = ((delta.x() > 0) ^ event->inverted()) ? "left" : "right";
    } else {
        params.action = ((delta.y() > 0) ^ event->inverted()) ? "up" : "down";
    }

    qDebug() << "wheelEvent" << params.action.c_str() << params.modifier.c_str() << params.col << params.row;
    emit mouseInput(params);
    event->setAccepted(true);
}
