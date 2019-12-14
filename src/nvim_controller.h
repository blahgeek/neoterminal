#pragma once

#include <QObject>
#include <QWidget>
#include <QIODevice>
#include <QLineEdit>

#include <utility>
#include <memory>

class MsgpackRpc;
class NvimUIWidget;
class NvimUIState;

class NvimController: public QObject {
    Q_OBJECT;

private:

    std::unique_ptr<MsgpackRpc> rpc_;
    std::unique_ptr<NvimUIState> ui_state_;
    std::unique_ptr<NvimUIWidget> ui_widget_;

    bool attached_ = false;

public:
    NvimUIWidget* ui_widget() { return ui_widget_.get(); }

    NvimController(std::unique_ptr<QIODevice> io);

private slots:
    void send_attach_or_resize();

};
