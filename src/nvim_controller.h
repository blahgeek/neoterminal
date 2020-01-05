#pragma once

#include <QObject>
#include <QWidget>
#include <QIODevice>
#include <QThread>

#include <utility>
#include <memory>

#include <msgpack.hpp>

class MsgpackRpc;
class NvimUIWidget;
class NvimUICalc;

class NvimController: public QObject {
    Q_OBJECT;

private:

    QThread ui_calc_thread_;
    std::unique_ptr<MsgpackRpc> rpc_;
    std::unique_ptr<NvimUICalc> ui_calc_;
    std::unique_ptr<NvimUIWidget> ui_widget_;

    bool attached_ = false;

public:
    NvimUIWidget* ui_widget() { return ui_widget_.get(); }

    NvimController(std::unique_ptr<QIODevice> io);
    ~NvimController();

private slots:
    void send_attach_or_resize();
    void handle_notification(std::string const& method, msgpack::object const& params);

signals:
    void on_notification_redraw(msgpack::object const& params);

};
