#include <QDebug>
#include <QPainter>

#include <cmath>

#include "./term_widget.h"


#define ASCII_STRING " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"


TermWidget::TermWidget(QFont const& font):
font_(font), font_metrics_(font, this) {
    this->calculateGrid();
}

void TermWidget::calculateGrid() {
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
}

void TermWidget::setFont(QFont const& font) {
    font_ = font;
    font_metrics_ = QFontMetrics(font, this);

    qDebug() << "setFont: " << font_;
    this->calculateGrid();
}


void TermWidget::paintDebugGrid(QPaintEvent* event, QPainter* painter) {
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

void TermWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    this->paintDebugGrid(event, &painter);
}

void TermWidget::resizeEvent(QResizeEvent* event) {
    this->calculateGrid();
}
