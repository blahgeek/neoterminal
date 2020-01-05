#pragma once

#include <QWidget>
#include <QFont>
#include <QCache>
#include <QStaticText>
#include <QFontMetricsF>

#include "./msgpack_rpc.h"
#include "./nvim_ui_state.h"

unsigned int qHash(QColor);

class NvimUIWidget : public QWidget {
    Q_OBJECT;

private:
    QFont font_;
    QFontMetricsF font_metrics_;

    QSizeF cell_size_;
    QSize grid_size_;
    QPointF grid_offset_;

    QString im_preedit_text_;
    Qt::MouseButton pressed_mouse_btn_;

    std::shared_ptr<NvimUIState> state_;

    QCache<QPair<uint32_t, QString>, QStaticText> static_texts_;
    QCache<QColor, QPen> cache_pens_;

public:
    struct MouseInputParams {
        std::string button;
        std::string action;
        std::string modifier;
        int row, col;
    };

signals:
    void gridSizeChanged();
    void keyPressed(std::string vim_keycodes);
    void mouseInput(MouseInputParams params);

public slots:
    void updateState(std::shared_ptr<NvimUIState> state,
                     QRegion dirty_cells, bool defaults_updated);

private:

    void calculateGrid();
    void processMouseEvent(QMouseEvent* event);

    void paintDebugGrid(QPaintEvent* event, QPainter* painter);

public:
    NvimUIWidget(QWidget* parent=nullptr);

    void setFont(QFont const& font);
    QSize grid_size() const { return grid_size_; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent* event) override;
};
