#include <QApplication>
#include <QMainWindow>
#include <QTcpSocket>

#include <chrono>
#include <memory>
#include <iostream>

#include "./nvim_controller.h"
#include "./nvim_ui_widget.h"

int main(int argc, char* argv[]) {

    qSetMessagePattern("%{time process} [%{function}:%{line}:%{type}] %{message}");

    QCoreApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta, true);

    QApplication app(argc, argv);

    QTcpSocket* socket = new QTcpSocket();
    socket->connectToHost(argv[1], atoi(argv[2]));

    NvimController nvim((std::unique_ptr<QIODevice>(socket)));
    nvim.ui_widget()->setFont(QFont("Fira Code", 11));
    nvim.ui_widget()->show();
    nvim.ui_widget()->setFocus();

    return app.exec();
}
