#include <QtGlobal>

#include <chrono>
#include <memory>
#include <iostream>

#include "./application.h"
#include "./nvim_ui_calc.h"
#include "./nvim_ui_widget.h"

int main(int argc, char* argv[]) {

    qSetMessagePattern("%{time process} [%{function}:%{line}:%{type}] %{message}");

    Application app(argc, argv);

    return app.exec();
}
