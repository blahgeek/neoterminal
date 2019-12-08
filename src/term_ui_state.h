#pragma once

#include <QObject>
#include <QColor>
#include <QSize>
#include <QRect>
#include <QRegion>

#include <unordered_map>
#include <vector>

#include <msgpack.hpp>

namespace msgpack::adaptor {

template<>
struct convert<QColor> {
    msgpack::object const& operator()(msgpack::object const& obj, QColor& optional_color) const;
};

}

class TermUIState : public QObject {
    Q_OBJECT;

public:
    using highlight_id_t = int;

    struct Highlight {
        TermUIState* term_ui_;  // for default_*_

        QColor foreground, background, special;
        bool reverse = false,
             italic = false,
             bold = false,
             strikethrough = false,
             underline = false,
             undercurl = false;
        int blend = 0;

        QColor const& effective_foreground() const;
        QColor const& effective_background() const;
        QColor const& effective_special() const;

        MSGPACK_DEFINE_MAP(foreground, background, special,
                           reverse, italic, bold, strikethrough, underline, undercurl,
                           blend);
    };

    friend struct Highlight;

private:
    QColor default_foreground_;
    QColor default_background_;
    QColor default_special_;

    std::unordered_map<highlight_id_t, Highlight> highlights_;
    Highlight default_highlight_;

    struct Cell {
        std::string text;
        highlight_id_t highlight_id = 0;

        QString contiguous_text;
        int contiguous_cols = 0;  // if negative: the start cols

        void reset();
        bool is_empty() const;
        bool is_whitespace() const;
    };

    std::vector<std::vector<Cell>> cells_;

    int width_ = 0, height_ = 0;
    QRegion dirty_cells_;

public:
    QSize size() const { return QSize(width_, height_); }
    Cell const& cell_at(int x, int y) { return cells_[y][x]; }
    Highlight const& highlight(highlight_id_t id) const;

public:
    TermUIState();

    void redraw(msgpack::object const& params);

signals:
    void updated(QRegion dirty_cells);

private:
    void handle_grid_resize(int grid, int width, int height);
    void handle_default_colors_set(QColor const& rgb_fg,
                                   QColor const& rgb_bg,
                                   QColor const& rgb_sp);
    void handle_hl_attr_define(highlight_id_t id,
                               struct Highlight rgb_attr);
    void handle_grid_line(int grid, int row, int col_start,
                          msgpack::object const& data);
    void handle_grid_clear(int grid);
    void handle_grid_scroll(int grid, int top, int bot, int left, int right, int rows, int cols);
    void handle_flush();

private:
    void refresh_contiguous_text(int row, int start, int end);
};
