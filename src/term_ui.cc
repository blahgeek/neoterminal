#include "./term_ui.h"

#include <QDebug>

namespace msgpack {
namespace adaptor {

template<>
struct convert<TermUI::OptionalColor> {
    msgpack::object const& operator()(msgpack::object const& obj, TermUI::OptionalColor& optional_color) const {
        if (obj.is_nil() || obj.type != msgpack::type::POSITIVE_INTEGER) {
            optional_color.is_present = false;
        } else {
            optional_color.is_present = true;
            int64_t rgb = obj.via.i64;
            optional_color.color = QColor(
                    (rgb >> 16) & 0xff,
                    (rgb >> 8) & 0xff,
                    (rgb) & 0xff
            );
        }
        return obj;
    }
};

}
}


void TermUI::redraw(msgpack::object const& params) {
    if (params.type != msgpack::type::ARRAY) {
        qWarning() << "Redraw with unknow params " << params.type;
        return;
    }

    for (int i = 0 ; i < params.via.array.size ; i += 1) {
        msgpack::object const* obj = params.via.array.ptr + i;
        if (obj->type != msgpack::type::ARRAY || obj->via.array.size == 0) {
            qWarning() << "Redraw with unknown params element " << params.type;
            continue;
        }

        if (obj->via.array.ptr[0].type != msgpack::type::STR) {
            qWarning() << "Redraw with unknown params element[0]";
            continue;
        }

        auto const& name = obj->via.array.ptr[0].via.str;

        if (strncmp(name.ptr, "grid_resize", name.size) == 0) {
            this->handle_grid_resize(
                    obj->via.array.ptr[1].as<int>(),
                    obj->via.array.ptr[2].as<int>(),
                    obj->via.array.ptr[3].as<int>());
        } else if (strncmp(name.ptr, "default_colors_set", name.size) == 0) {
            this->handle_default_colors_set(
                    obj->via.array.ptr[1].as<OptionalColor>(),
                    obj->via.array.ptr[2].as<OptionalColor>(),
                    obj->via.array.ptr[3].as<OptionalColor>());
        } else if (strncmp(name.ptr, "hl_attr_define", name.size) == 0) {
            this->handle_hl_attr_define(
                    obj->via.array.ptr[1].as<highlight_id_t>(),
                    obj->via.array.ptr[2].as<Hightlight>());
        } else if (strncmp(name.ptr, "grid_line", name.size) == 0) {
            this->handle_grid_line(
                    obj->via.array.ptr[1].as<int>(),
                    obj->via.array.ptr[2].as<int>(),
                    obj->via.array.ptr[3].as<int>(),
                    obj->via.array.ptr[4]);
        } else {
            qWarning() << "Ignore draw event " << QString(QByteArray(name.ptr, name.size));
        }
    }
}

void TermUI::handle_grid_resize(int grid, int width, int height) {
    assert(grid == 1);

    qDebug() << "handle_grid_resize " << width << ", " << height;

    width_ = width;
    height_ = height;

    cells_.resize(height_);
    for (int i = 0 ; i < height ; i += 1)
        cells_[i].resize(width_);
}

void TermUI::handle_default_colors_set(OptionalColor const& fg,
                                       OptionalColor const& bg,
                                       OptionalColor const& sp) {
    qDebug() << "handle_default_colors_set "
        << fg.color << ", "
        << bg.color << ", "
        << sp.color;

    default_foreground_ = fg;
    default_background_ = bg;
    default_special_ = sp;
}

void TermUI::handle_hl_attr_define(highlight_id_t id, Hightlight attr) {
    qDebug() << "hl_attr_define " << id;

    highlights_[id] = attr;
}

void TermUI::handle_grid_line(int grid, int row, int col_start, msgpack::object const& data) {
    assert(grid == 1);
    qDebug() << "handle_grid_line " << grid << ", " << row << ", " << col_start;

    assert(data.type == msgpack::type::ARRAY);
    for (int i = 0 ; i < data.via.array.size ; i += 1) {
        msgpack::object const& elem = data.via.array.ptr[i];
        assert(elem.type == msgpack::type::ARRAY);
        assert(elem.via.array.size >= 1);
        assert(elem.via.array.size <= 3);
    }
}
