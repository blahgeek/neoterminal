#include <QVBoxLayout>
#include <QDebug>

#include "./nvim_controller.h"
#include "./msgpack_rpc.h"
#include "./nvim_ui_state.h"
#include "./nvim_ui_widget.h"


NvimController::NvimController(std::unique_ptr<QIODevice> io) {
    rpc_.reset(new MsgpackRpc(std::move(io)));
    ui_state_.reset(new NvimUIState);
    ui_widget_.reset(new NvimUIWidget(ui_state_.get()));

    QObject::connect(rpc_.get(), &MsgpackRpc::on_notification,
                     [this](std::string const& method, msgpack::object const& params) {
                         if (method == "redraw")
                             ui_state_->redraw(params);
                     });
    QObject::connect(ui_widget_.get(), &NvimUIWidget::keyPressed,
                     [this](std::string const& vim_keycodes) {
                         rpc_->call("nvim_input", vim_keycodes);
                     });

    QObject::connect(ui_widget_.get(), &NvimUIWidget::gridSizeChanged,
                     this, &NvimController::send_attach_or_resize);
    QObject::connect(ui_state_.get(), &NvimUIState::updated,
                     ui_widget_.get(), &NvimUIWidget::redrawCells);
    QObject::connect(ui_state_.get(), &NvimUIState::fontChangeRequested,
                     ui_widget_.get(), &NvimUIWidget::setFont);
}

void NvimController::send_attach_or_resize() {
    QSize grid_size = ui_widget_->grid_size();

    qDebug() << "Grid size" << grid_size;
    if (attached_) {
        rpc_->call("nvim_ui_try_resize",
                   grid_size.width(), grid_size.height());
    } else {
        rpc_->call("nvim_ui_attach",
                   grid_size.width(), grid_size.height(),
                   std::map<std::string, bool>({{"ext_linegrid", true}}));
        attached_ = true;
    }
}
