#pragma once

#include <QObject>
#include <QWidget>
#include <QIODevice>

#include <utility>
#include <memory>

class MsgpackRpc;
class TermUIWidget;
class TermUIState;

class TermWidget: public QWidget {
    Q_OBJECT;

private:

    std::unique_ptr<MsgpackRpc> rpc_;
    std::unique_ptr<TermUIState> ui_state_;
    std::unique_ptr<TermUIWidget> ui_widget_;

    bool attached_ = false;

public:
    TermUIWidget* term_ui_widget() { return ui_widget_.get(); }

    TermWidget(std::unique_ptr<QIODevice> io);

private slots:
    void send_attach_or_resize();

};
