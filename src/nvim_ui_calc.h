#pragma once

#include <QObject>
#include <QColor>
#include <QSize>
#include <QRect>
#include <QRegion>

#include <unordered_map>
#include <vector>

#include <msgpack.hpp>
#include "./nvim_ui_state.h"

class NvimUICalc : public QObject {
    Q_OBJECT;

public:
    using highlight_id_t = int;

    using Highlight = NvimUIState::Highlight;
    using Modeinfo = NvimUIState::Modeinfo;

private:
    QColor default_foreground_;
    QColor default_background_;
    QColor default_special_;

    std::unordered_map<highlight_id_t, std::shared_ptr<Highlight>> highlights_;

    struct InternalCell {
        std::string text;
        highlight_id_t highlight_id = 0;

        QString contiguous_text;
        int contiguous_cols = 0;  // if negative: the start cols

        void reset();
        bool is_empty() const;
        bool is_whitespace() const;
    };

    std::vector<std::vector<InternalCell>> cells_;
    QRegion dirty_cells_;
    bool dirty_defaults_ = false;

    int width_ = 0, height_ = 0;

    // modes & cursors
    std::string mode_;
    int mode_idx_ = -1;
    std::vector<Modeinfo> modeinfos_;
    QPoint cursor_ = QPoint(-1, -1);
    QPoint cursor_saved_on_busy_ = QPoint(-1, -1);

public:
    NvimUICalc();

    void redraw(msgpack::object const& params);

signals:
    void updated(std::shared_ptr<NvimUIState> state,
                 QRegion dirty_cells, bool defaults_updated);
    void fontChangeRequested(QFont font);

private:
    void handle_grid_resize(int grid, int width, int height);
    void handle_default_colors_set(QColor const& rgb_fg,
                                   QColor const& rgb_bg,
                                   QColor const& rgb_sp);
    void handle_hl_attr_define(highlight_id_t id,
                               Highlight rgb_attr);
    void handle_grid_line(int grid, int row, int col_start,
                          msgpack::object const& data);
    void handle_grid_clear(int grid);
    void handle_grid_scroll(int grid, int top, int bot, int left, int right, int rows, int cols);
    void handle_flush();

    void handle_mode_info_set(bool cursor_style_enabled,
                              std::vector<Modeinfo> mode_infos);
    void handle_mode_change(std::string const& mode, int mode_idx);
    void handle_grid_cursor_goto(int grid, int row, int col);
    void handle_option_set(std::string const& name, msgpack::object const& value);

    void handle_busy_start();
    void handle_busy_stop();

private:
    void refresh_contiguous_text(int row, int start, int end);
    void refresh_cursor(QPoint new_pos);
};
