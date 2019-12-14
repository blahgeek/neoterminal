#pragma once

#include <QApplication>

#include "./msgpack_rpc.h"
#include "./nvim_ui_widget.h"
#include "./nvim_controller.h"


class Application: public QApplication {
    Q_OBJECT;

private:
    std::unique_ptr<NvimController> nvim_controller_;

public:
    Application(int argc, char* argv[]);
};
