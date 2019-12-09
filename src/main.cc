#include <QApplication>
#include <QMainWindow>
#include <QTcpSocket>

#include <chrono>
#include <memory>
#include <iostream>

#include "./term_controller.h"
#include "./term_ui_widget.h"

int main(int argc, char* argv[]) {

    qSetMessagePattern("%{time process} [%{function}:%{line}:%{type}] %{message}");

    QCoreApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta, true);

    QApplication app(argc, argv);

    QTcpSocket* socket = new QTcpSocket();
    socket->connectToHost(argv[1], atoi(argv[2]));

    TermController term((std::unique_ptr<QIODevice>(socket)));
    term.term_ui_widget()->setFont(QFont("Fira Code", 11));
    term.term_ui_widget()->show();
    term.term_ui_widget()->setFocus();

    return app.exec();
}
