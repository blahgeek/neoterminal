#include <QVBoxLayout>
#include <QDebug>

#include "./term_widget.h"
#include "./msgpack_rpc.h"
#include "./term_ui_state.h"
#include "./term_ui_widget.h"


TermWidget::TermWidget(std::unique_ptr<QIODevice> io) {
    rpc_.reset(new MsgpackRpc(std::move(io)));
    ui_state_.reset(new TermUIState);
    ui_widget_.reset(new TermUIWidget(ui_state_.get()));

    QVBoxLayout* layout = new QVBoxLayout(this);
    // layout->setSpacing(0);
    // layout->setMargin(0);
    layout->addWidget(ui_widget_.get());
    this->setLayout(layout);

    ui_widget_->setFocus();
    QObject::connect(rpc_.get(), &MsgpackRpc::on_notification,
                     [this](std::string const& method, msgpack::object const& params) {
                         if (method == "redraw")
                             ui_state_->redraw(params);
                     });
    QObject::connect(ui_widget_.get(), &TermUIWidget::keyPressed,
                     [this](std::string const& vim_keycodes) {
                         rpc_->call("nvim_input", vim_keycodes);
                     });

    QObject::connect(ui_widget_.get(), &TermUIWidget::gridSizeChanged,
                     this, &TermWidget::send_attach_or_resize);
    QObject::connect(ui_state_.get(), &TermUIState::updated,
                     ui_widget_.get(), &TermUIWidget::redrawCells);

    // msgpack::zone zone;
    // msgpack::type::tuple<int, int, std::map<std::string, bool>> params(80, 40, std::map<std::string, bool>({{"ext_linegrid", true}}));
    // msgpack::object obj(params, zone);
    //
    // rpc_->call("nvim_ui_attach", obj);
}

void TermWidget::send_attach_or_resize() {
    QSize grid_size = ui_widget_->grid_size();

    // msgpack::zone zone;
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
