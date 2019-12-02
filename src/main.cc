#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QFont>
#include <QTextLayout>
#include <QStaticText>
#include <QOpenGLWidget>
#include <QTcpSocket>

#include <chrono>
#include <memory>
#include <iostream>

#include "./term_widget.h"
#include "./msgpack_rpc.h"


#define CONTENT_TEXT "Hello world() 你好世界OMG ++ -> 🦃🍗"
// #define CONTENT_TEXT "Hello world() 你好世界OMG ++ -> "
#define CONTENT_REPEAT 1000


class MyWindow: public QOpenGLWidget {

private:
    QFont font = QFont("Fira Code", 14);

public:

    void paintGL() override {
        // this->draw_naive();
        // this->draw_steps();
        this->draw_static();
    }

    void draw_naive() {
        QPainter painter(this);
        painter.setFont(font);

        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0 ; i < CONTENT_REPEAT ; i += 1) {
            painter.drawText(QPointF(100.f, 100.f), CONTENT_TEXT);
        }
        auto t1 = std::chrono::steady_clock::now();
        std::cerr << "DRAW NAIVE: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << std::endl;
    }

    void draw_steps() {
        QPainter painter(this);

        auto t0 = std::chrono::steady_clock::now();
        std::unique_ptr<QTextLayout> layout;
        for (int i = 0 ; i < CONTENT_REPEAT ; i += 1) {
            layout.reset(new QTextLayout(QString(CONTENT_TEXT), font, this));
            layout->beginLayout();
            QTextLine line = layout->createLine();
            line.setLineWidth(1000);
            layout->endLayout();

            layout->glyphRuns();
        }

        auto t1 = std::chrono::steady_clock::now();
        for (int i = 0 ; i < CONTENT_REPEAT ; i += 1) {
            layout->draw(&painter, QPointF(100.f, 100.f));
        }
        auto t2 = std::chrono::steady_clock::now();
        std::cerr << "LAYOUT: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << std::endl;
        std::cerr << "DRAW: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << std::endl;
    }

    void draw_static() {
        QPainter painter(this);

        auto t0 = std::chrono::steady_clock::now();
        std::unique_ptr<QStaticText> static_text;
        for (int i = 0 ; i < CONTENT_REPEAT ; i += 1) {
            static_text.reset(new QStaticText(CONTENT_TEXT));
            static_text->setPerformanceHint(QStaticText::PerformanceHint::AggressiveCaching);
            static_text->prepare(QTransform(), font);
        }

        auto t1 = std::chrono::steady_clock::now();
        for (int i = 0 ; i < CONTENT_REPEAT ; i += 1) {
            painter.drawStaticText(QPointF(100.f, 100.f), *static_text);
        }
        auto t2 = std::chrono::steady_clock::now();

        std::cerr << "LAYOUT: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count() << std::endl;
        std::cerr << "DRAW: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << std::endl;
    }
};


int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // TermWidget term(QFont("Fira Code", 14));
    // term.show();

    // MyWindow window;
    // window.resize(QSize(1000, 1000));
    // window.show();
    // window.draw_naive();
    QTcpSocket* socket = new QTcpSocket();
    socket->connectToHost(argv[1], atoi(argv[2]));

    MsgpackRpc rpc ((std::unique_ptr<QIODevice>(socket)));

    msgpack::zone zone;
    msgpack::type::tuple<int, int, std::map<std::string, bool>> params(30, 30, std::map<std::string, bool>());
    msgpack::object obj(params, zone);

    rpc.call("nvim_ui_attach", obj);

    return app.exec();
}
