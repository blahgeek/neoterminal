#pragma once


#include <memory>

#include <QColor>
#include <QPoint>
#include <QSize>

#include <msgpack.hpp>

namespace msgpack::adaptor {

template<>
struct convert<QColor> {
    msgpack::object const& operator()(msgpack::object const& obj, QColor& optional_color) const;
};

}

struct NvimUIState {

    struct Highlight {
        QColor foreground, background, special;
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

    struct Modeinfo {
        std::string cursor_shape = "block"; // block, horizontal, vertical
        int cell_percentage = 100;
        // TODO: blink* not implemented
        // TODO: attr_id not implemented
        std::string short_name;
        std::string name;

        MSGPACK_DEFINE_MAP(cursor_shape, cell_percentage,
                           short_name, name);
    };

    struct Cell {
        std::shared_ptr<Highlight> highlight;
        QString contiguous_text;
        int contiguous_cols;
    };

    QColor default_foreground;
    QColor default_background;
    QColor default_special;

    QPoint cursor;
    QSize size = QSize(0, 0);
    Modeinfo modeinfo;

    std::vector<std::vector<Cell>> cells;

};
