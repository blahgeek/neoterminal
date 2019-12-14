#pragma once

#include <QWidget>
#include <QFont>
#include <QCache>
#include <QStaticText>
#include <QFontMetricsF>

#include "./msgpack_rpc.h"
#include "./nvim_ui_state.h"

class NvimUIWidget : public QWidget {
    Q_OBJECT;

private:
    QFont font_;
    QFontMetricsF font_metrics_;

    QSizeF cell_size_;
    QSize grid_size_;
    QPointF grid_offset_;

    QString im_preedit_text_;

    NvimUIState* state_; // not owned

    QCache<QPair<uint32_t, QString>, QStaticText> static_texts_;

signals:
    void gridSizeChanged();
    void keyPressed(std::string vim_keycodes);

public slots:
    void redrawCells(QRegion dirty_cells);

private:

    void calculateGrid();

    void paintDebugGrid(QPaintEvent* event, QPainter* painter);

public:
    NvimUIWidget(NvimUIState* state,
                 QWidget* parent=nullptr);

    void setFont(QFont const& font);
    QSize grid_size() const { return grid_size_; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent* event) override;
};