#include <QVBoxLayout>
#include <QDebug>

#include <iostream>

#include "./nvim_controller.h"
#include "./msgpack_rpc.h"
#include "./nvim_ui_calc.h"
#include "./nvim_ui_widget.h"


NvimController::NvimController(std::unique_ptr<QIODevice> io) {
    rpc_.reset(new MsgpackRpc(std::move(io)));
    ui_calc_.reset(new NvimUICalc);
    ui_widget_.reset(new NvimUIWidget);

    ui_calc_->moveToThread(&ui_calc_thread_);

    QObject::connect(rpc_.get(), &MsgpackRpc::on_notification,
                     this, &NvimController::handle_notification);
    QObject::connect(rpc_.get(), &MsgpackRpc::on_close,
                     ui_widget_.get(), &QWidget::close);

    QObject::connect(ui_widget_.get(), &NvimUIWidget::keyPressed,
                     [this](std::string const& vim_keycodes) {
                         rpc_->call("nvim_input", vim_keycodes);
                     });
    QObject::connect(ui_widget_.get(), &NvimUIWidget::mouseInput,
                     [this](NvimUIWidget::MouseInputParams params) {
                         rpc_->call("nvim_input_mouse",
                                    params.button, params.action, params.modifier,
                                    0, params.row, params.col);
                     });

    QObject::connect(ui_widget_.get(), &NvimUIWidget::gridSizeChanged,
                     this, &NvimController::send_attach_or_resize);

    QObject::connect(ui_calc_.get(), &NvimUICalc::updated,
                     ui_widget_.get(), &NvimUIWidget::updateState);
    QObject::connect(ui_calc_.get(), &NvimUICalc::fontChangeRequested,
                     ui_widget_.get(), &NvimUIWidget::setFont);
    QObject::connect(this, &NvimController::on_notification_redraw,
                     ui_calc_.get(), &NvimUICalc::redraw);

    ui_calc_thread_.start();
}

NvimController::~NvimController() {
    ui_calc_thread_.quit();
    ui_calc_thread_.wait();
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
        rpc_->call("nvim_set_client_info",
                   std::string("neoterminal"),
                   std::map<std::string, std::string>(),
                   std::string("ui"),
                   std::map<std::string, std::string>(),
                   std::map<std::string, std::string>());
        attached_ = true;
    }
}

void NvimController::handle_notification(std::string const& method, msgpack::object const& params) {
    if (method == "redraw")
        emit on_notification_redraw(params);
}
