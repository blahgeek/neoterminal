#pragma once

#include <QObject>
#include <QColor>

#include <unordered_map>
#include <vector>

#include <msgpack.hpp>

class TermUI : QObject {
    Q_OBJECT;

public:
    using highlight_id_t = int;

    struct OptionalColor {
        bool is_present = false;
        QColor color;
    };

    struct Hightlight {
        OptionalColor foreground, background, special;
        bool reverse = false,
             italic = false,
             bold = false,
             strikethrough = false,
             underline = false,
             undercurl = false;
        int blend = 0;

        MSGPACK_DEFINE_MAP(foreground, background, special,
                           reverse, italic, bold, strikethrough, underline, undercurl,
                           blend);
    };

private:
    OptionalColor default_foreground_;
    OptionalColor default_background_;
    OptionalColor default_special_;

    std::unordered_map<highlight_id_t, Hightlight> highlights_;

    struct Cell {
        static constexpr int CELL_TEXT_MAXLEN = 8;

        char cell_text[CELL_TEXT_MAXLEN] = "\0";
        highlight_id_t highlight_id;

        QString contiguous_text;
        int continuous_cols = 0;
    };

    std::vector<std::vector<Cell>> cells_;

    int width_ = 0, height_ = 0;
    highlight_id_t last_highlight_id_;

public:
    void redraw(msgpack::object const& params);

private:
    void handle_grid_resize(int grid, int width, int height);
    void handle_default_colors_set(OptionalColor const& rgb_fg,
                                   OptionalColor const& rgb_bg,
                                   OptionalColor const& rgb_sp);
    void handle_hl_attr_define(highlight_id_t id,
                               struct Hightlight rgb_attr);
    void handle_grid_line(int grid, int row, int col_start,
                          msgpack::object const& data);
};
