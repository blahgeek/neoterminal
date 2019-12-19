#include <QtGlobal>

#include <chrono>
#include <memory>
#include <iostream>

#include "./application.h"

int main(int argc, char* argv[]) {

    qSetMessagePattern("%{time process} [%{function}:%{line}:%{type}] %{message}");

    Application app(argc, argv);

    return app.exec();
}
