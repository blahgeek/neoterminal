#pragma once

#include <QObject>
#include <QIODevice>

#include <memory>
#include <utility>
#include <functional>
#include <unordered_map>

#include <msgpack.hpp>

// for single-thread only
class MsgpackRpc: public QObject {
    Q_OBJECT;

public:
    MsgpackRpc(std::unique_ptr<QIODevice> iodevice);

    // callback: (erro_code, obj)
    using callback_t = std::function<void(int, msgpack::object const&)>;

    int call(std::string const& method, msgpack::object const& params,
             callback_t callback=callback_t());
    bool is_open();

signals:
    void on_notification(std::string const& method, msgpack::object const& params);

private:
    std::unique_ptr<QIODevice> iodevice_;

    std::unordered_map<uint32_t, callback_t> callbacks_;
    msgpack::unpacker unpacker_;

    uint32_t counter_ = 0;

private:
    void do_read();
};
