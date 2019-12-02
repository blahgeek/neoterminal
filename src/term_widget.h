#pragma once

#include <QWidget>
#include <QFont>
#include <QFontMetricsF>

class TermWidget : public QWidget {

private:
    QFont font_;
    QFontMetricsF font_metrics_;

    QSizeF cell_size_;
    QSize grid_size_;
    QPointF grid_offset_;

private:

    void calculateGrid();

    void paintDebugGrid(QPaintEvent* event, QPainter* painter);

public:
    TermWidget(QFont const& font);

    void setFont(QFont const& font);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};
