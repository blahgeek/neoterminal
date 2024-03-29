#include "./nvim_ui_calc.h"

#include <QDebug>
#include <QFont>
#include <QRegularExpression>

#include <algorithm>
#include <functional>
#include <array>
#include <tuple>
#include <type_traits>
#include <chrono>
#include <iostream>

namespace {
    // haha.. magic!

    template <typename T>
    struct nvim_ui_handle_helper;

    template <typename... Args>
    struct nvim_ui_handle_helper<void(NvimUICalc::*)(Args...)> {
        static const size_t nargs = sizeof...(Args);

        using fn_t = void(NvimUICalc::*)(Args...);

        template <size_t I>
        struct arg_t {
            using type = typename std::tuple_element<I, std::tuple<typename std::decay_t<Args>...>>::type;
        };

        template <size_t I>
        static typename arg_t<I>::type arg(msgpack::object_array const& array) {
            typename arg_t<I>::type ret;
            if (I < array.size)
                ret = array.ptr[I].template as<typename arg_t<I>::type>();
            return std::move(ret);
        }

        template <size_t... Ids>
        static decltype(auto) call_internal(NvimUICalc* nvimui, fn_t fn, msgpack::object_array const& array,
                                            std::index_sequence<Ids...>) {
            return (nvimui->*fn)(arg<Ids>(array)...);
        }

        static decltype(auto) call(NvimUICalc* nvimui, fn_t fn, msgpack::object_array const& array) {
            return call_internal(nvimui, fn, array,
                                 std::index_sequence_for<Args...>());
        }
    };

}

msgpack::object const& msgpack::adaptor::convert<QColor>::operator()(msgpack::object const& obj, QColor& color) const {
    // otherwise, keep color invalid
    if (obj.type == msgpack::type::POSITIVE_INTEGER) {
        int64_t rgb = obj.via.i64;
        color = QColor(
                (rgb >> 16) & 0xff,
                (rgb >> 8) & 0xff,
                (rgb) & 0xff
        );
    }
    return obj;
}

void NvimUICalc::InternalCell::reset() {
    this->text.clear();
    this->highlight_id = 0;
    this->contiguous_cols = 0;
}

bool NvimUICalc::InternalCell::is_whitespace() const {
    return this->text == " ";
}

bool NvimUICalc::InternalCell::is_empty() const {
    return this->text.empty();
}

NvimUICalc::NvimUICalc() = default;

void NvimUICalc::redraw(msgpack::object const& params) {
    assert(params.type == msgpack::type::ARRAY);

    auto t0 = std::chrono::steady_clock::now();

    for (int i = 0 ; i < params.via.array.size ; i += 1) {
        msgpack::object const* obj = params.via.array.ptr + i;

        assert(obj->type == msgpack::type::ARRAY);
        assert(obj->via.array.size > 1);

        auto const& objarray = obj->via.array;
        assert(objarray.ptr[0].type == msgpack::type::STR);

        auto const& name = objarray.ptr[0].via.str;

        bool handled = false;

#define TRY_HANDLE(NAME) do { \
    if (strncmp(name.ptr, #NAME, name.size) == 0) { \
        handled = true; \
        for (int j = 1 ; j < objarray.size ; j += 1) { \
            assert(objarray.ptr[j].type == msgpack::type::ARRAY); \
            nvim_ui_handle_helper<decltype(&NvimUICalc::handle_ ## NAME)>::call( \
                this, &NvimUICalc::handle_ ## NAME, objarray.ptr[j].via.array); \
        } \
    } \
} while (0)

        TRY_HANDLE(grid_resize);
        TRY_HANDLE(default_colors_set);
        TRY_HANDLE(hl_attr_define);
        TRY_HANDLE(grid_line);
        TRY_HANDLE(grid_clear);
        TRY_HANDLE(grid_scroll);
        TRY_HANDLE(flush);
        TRY_HANDLE(mode_info_set);
        TRY_HANDLE(mode_change);
        TRY_HANDLE(grid_cursor_goto);
        TRY_HANDLE(busy_start);
        TRY_HANDLE(busy_stop);
        TRY_HANDLE(option_set);

#undef TRY_HANDLE

        if (!handled) {
            qWarning() << "Ignore draw event " << QString(QByteArray(name.ptr, name.size));
            std::cerr << *obj << std::endl;
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    qDebug() << "Parsing redraw event costs" << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << "us";
}

void NvimUICalc::handle_grid_resize(int grid, int width, int height) {
    assert(grid == 1);

    qDebug() << "handle_grid_resize" << width << height;

    width_ = width;
    height_ = height;

    cells_.resize(height_);
    for (int i = 0 ; i < height ; i += 1) {
        cells_[i].resize(width_);
        for (auto& cell: cells_[i])
            cell.reset();
    }

    dirty_cells_ |= QRect(0, 0, width, height);
}

void NvimUICalc::handle_default_colors_set(QColor const& fg,
                                            QColor const& bg,
                                            QColor const& sp) {
    qDebug() << "handle_default_colors_set" << fg << bg << sp;

    default_foreground_ = fg;
    default_background_ = bg;
    default_special_ = sp;

    dirty_cells_ |= QRect(0, 0, width_, height_);
    dirty_defaults_ = true;
}

void NvimUICalc::handle_hl_attr_define(highlight_id_t id, Highlight attr) {
    qDebug() << "handle_hl_attr_define" << id;

    highlights_[id] = std::shared_ptr<Highlight>(new Highlight(attr));
}

void NvimUICalc::handle_grid_line(int grid, int row, int col_start, msgpack::object const& data) {
    assert(grid == 1);
    assert(row >= 0 && row < height_);
    assert(col_start >= 0 && col_start < width_);
    qDebug() << "handle_grid_line" << grid << row << col_start;

    auto& cells_row = this->cells_[row];

    highlight_id_t last_highlight_id;
    int col = col_start;

    assert(data.type == msgpack::type::ARRAY);
    for (int i = 0 ; i < data.via.array.size ; i += 1) {
        msgpack::object const& elem = data.via.array.ptr[i];
        assert(elem.type == msgpack::type::ARRAY);
        assert(elem.via.array.size >= 1);
        assert(elem.via.array.size <= 3);

        assert(elem.via.array.ptr[0].type == msgpack::type::STR);
        auto const& text = elem.via.array.ptr[0];

        if (elem.via.array.size >= 2) {
            assert(elem.via.array.ptr[1].type == msgpack::type::POSITIVE_INTEGER);
            last_highlight_id = elem.via.array.ptr[1].via.i64;
        }

        int repeat = 1;
        if (elem.via.array.size >= 3) {
            assert(elem.via.array.ptr[2].type == msgpack::type::POSITIVE_INTEGER);
            repeat = elem.via.array.ptr[2].via.i64;
        }

        for (int j = 0 ; j < repeat ; j += 1) {
            assert(col < width_);

            InternalCell& cell = cells_row[col];
            cell.text = std::string(text.via.str.ptr, text.via.str.size);
            cell.highlight_id = last_highlight_id;

            col += 1;
        }
    }

    this->refresh_contiguous_text(row, col_start, col);
}

void NvimUICalc::refresh_contiguous_text(int row, int start, int end) {
    assert(end >= start);
    assert(row >= 0 && row < height_);
    auto& cells_row = this->cells_[row];

    // expand to all non-whitelist cells
    while (start > 0 && !cells_row[start-1].is_whitespace())
        start -= 1;
    while (end < width_ && !cells_row[end].is_whitespace())
        end += 1;

    int x_start = start;
    if (x_start < width_ && cells_row[x_start].contiguous_cols < 0)
        x_start += cells_row[x_start].contiguous_cols;

    int x_end = end;
    if (x_end < width_ && cells_row[x_end].contiguous_cols < 0) {
        x_end += cells_row[x_end].contiguous_cols;
        assert(x_end >= 0);
        assert(cells_row[x_end].contiguous_cols > 0);
        x_end += cells_row[x_end].contiguous_cols;
        assert(x_end > end);
    }

    if (!(x_start < x_end))
        return;

    dirty_cells_ |= QRect(x_start, row, x_end - x_start, 1);

    int anchor_x = x_start;
    std::string contiguous_utf8;
    for (int x = x_start ; x <= x_end ; x += 1) {   // <= x_end
        if (x == x_end
            || cells_row[x].is_whitespace()
            || cells_row[anchor_x].is_whitespace()
            || (x > 0 && cells_row[x-1].is_empty())
            || QPoint(x, row) == cursor_
            || (QPoint(anchor_x, row) == cursor_ && !cells_row[x].is_empty())  // double width
            || cells_row[x].highlight_id != cells_row[anchor_x].highlight_id) {
            // if it's whitelist, keep contiguous_cols = 0
            if (!cells_row[anchor_x].is_whitespace() && !cells_row[anchor_x].is_empty()) {
                cells_row[anchor_x].contiguous_text = QString::fromUtf8(contiguous_utf8.data(), contiguous_utf8.size());
                cells_row[anchor_x].contiguous_cols = x - anchor_x;
            }

            anchor_x = x;
            contiguous_utf8.clear();
        }

        if (x < x_end) {
            cells_row[x].contiguous_cols = anchor_x - x;  // negative
            contiguous_utf8 += cells_row[x].text;
        }
    }
}

void NvimUICalc::handle_grid_clear(int grid) {
    assert(grid == 1);

    qDebug() << "handle_grid_clear";

    for (auto& row: cells_)
        for (auto& cell: row)
            cell.reset();

    dirty_cells_ |= QRect(0, 0, width_, height_);
}

void NvimUICalc::handle_grid_scroll(int grid, int top, int bot, int left, int right, int rows, int cols) {
    assert(grid == 1);
    assert(cols == 0);
    assert(rows != 0);

    qDebug() << "handle_grid_scroll" << top << bot << left << right << rows << cols;

    dirty_cells_ |= QRect(left, top, right-left, bot-top);

    if (rows > 0) {
        for (int y = top ; y < bot ; y += 1) {
            int dst_y = y - rows;
            if (dst_y < 0)
                continue;
            for (int x = left ; x < right ; x += 1) {
                assert(x >= 0 && x < width_);
                assert(y >= 0 && y < height_);
                cells_[dst_y][x] = cells_[y][x];
                cells_[y][x].reset();
            }
        }
    } else {
        for (int y = bot - 1 ; y >= top ; y -= 1) {
            int dst_y = y - rows;
            if (dst_y >= bot)
                continue;
            for (int x = left ; x < right ; x += 1) {
                assert(x >= 0 && x < width_);
                assert(y >= 0 && y < height_);
                cells_[dst_y][x] = cells_[y][x];
                cells_[y][x].reset();
            }
        }
    }

    for (int y = std::min(top, top - rows) ; y < std::max(bot, bot - rows) ; y += 1) {
        if (y < 0 || y >= height_)
            continue;
        if (left > 0)
            this->refresh_contiguous_text(y, left, left);
        if (right < width_)
            this->refresh_contiguous_text(y, right, right);
    }
}

void NvimUICalc::handle_flush() {
    qDebug() << "handle_flush";

    std::shared_ptr<NvimUIState> state(new NvimUIState);

    state->default_background = default_background_;
    state->default_foreground = default_foreground_;
    state->default_special = default_special_;

    state->cursor = cursor_;
    state->size = QSize(width_, height_);
    state->modeinfo = (mode_idx_ >= 0 && mode_idx_ < modeinfos_.size()) ? modeinfos_[mode_idx_] : Modeinfo();

    state->cells.resize(cells_.size());
    for (size_t i = 0 ; i < cells_.size() ; i += 1) {
        state->cells[i].resize(cells_[i].size());
        for (size_t j = 0 ; j < cells_[i].size() ; j += 1) {
            state->cells[i][j].contiguous_text = cells_[i][j].contiguous_text;
            state->cells[i][j].contiguous_cols = cells_[i][j].contiguous_cols;
            auto it = highlights_.find(cells_[i][j].highlight_id);
            if (it != highlights_.end())
                state->cells[i][j].highlight = it->second;
        }
    }

    emit updated(state, dirty_cells_, dirty_defaults_);

    dirty_cells_ = QRegion(0, 0, 0, 0);
    dirty_defaults_ = false;
}


void NvimUICalc::handle_mode_info_set(bool cursor_style_enabled,
                                       std::vector<Modeinfo> mode_infos) {
    qDebug() << "handle_mode_info_set";

    if (!cursor_style_enabled)
        modeinfos_.clear();
    else
        modeinfos_ = std::move(mode_infos);

    this->refresh_cursor(cursor_);
}

void NvimUICalc::handle_mode_change(std::string const& mode, int mode_idx) {
    qDebug() << "handle_mode_change" << mode.c_str() << mode_idx;

    mode_ = mode;
    mode_idx_ = mode_idx;

    this->refresh_cursor(cursor_);
}

void NvimUICalc::handle_grid_cursor_goto(int grid, int row, int col) {
    assert(grid == 1);
    qDebug() << "handle_grid_cursor_goto" << row << col;

    if (cursor_saved_on_busy_.x() >= 0 && cursor_saved_on_busy_.y() >= 0)
        cursor_saved_on_busy_ = QPoint(col, row);
    else
        this->refresh_cursor(QPoint(col, row));
}

void NvimUICalc::handle_busy_start() {
    qDebug() << "handle_busy_start";

    cursor_saved_on_busy_ = cursor_;
    this->refresh_cursor(QPoint(-1, -1));
}

void NvimUICalc::handle_busy_stop() {
    qDebug() << "handle_busy_stop" << cursor_saved_on_busy_;

    this->refresh_cursor(cursor_saved_on_busy_);
    cursor_saved_on_busy_ = QPoint(-1, -1);
}

void NvimUICalc::refresh_cursor(QPoint new_pos) {
    QPoint old_pos = cursor_;
    cursor_ = new_pos;

    if (old_pos != new_pos && old_pos.x() >= 0 && old_pos.y() >= 0 && old_pos.x() < width_ && old_pos.y() < height_)
        this->refresh_contiguous_text(old_pos.y(), old_pos.x(), old_pos.x() + 1);

    if (new_pos.x() >= 0 && new_pos.y() >= 0 && new_pos.x() < width_ && new_pos.y() < height_)
        this->refresh_contiguous_text(new_pos.y(), new_pos.x(), new_pos.x() + 1);
}

namespace {
    const QRegularExpression FONT_REGEX("^([\\w\\s]+),?(\\d*)$");
}

void NvimUICalc::handle_option_set(std::string const& name, msgpack::object const& value) {
    qDebug() << "handle_option_set" << name.c_str();

    if (name == "guifont" && value.type == msgpack::type::STR) {
        QString value_str = QString::fromUtf8(value.via.str.ptr, value.via.str.size);
        auto match = FONT_REGEX.match(value_str);
        if (match.hasMatch()) {
            QFont font;
            font.setFamily(match.captured(1));
            bool size_valid = false;
            int size = match.captured(2).toInt(&size_valid);
            if (size_valid)
                font.setPointSize(size);
            qDebug() << value_str << font;
            emit fontChangeRequested(font);
        }
    }
}
