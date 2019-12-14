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

    template <typename ... Args>
    int call(std::string const& method, callback_t callback, Args const&... args);

    template <typename ... Args>
    int call(std::string const& method, Args const&... args);

    bool is_open();

signals:
    void on_notification(std::string const& method, msgpack::object const& params);
    void on_close();

private:
    std::unique_ptr<QIODevice> iodevice_;

    std::unordered_map<uint32_t, callback_t> callbacks_;
    msgpack::unpacker unpacker_;

    uint32_t counter_ = 0;

private:
    void do_read();

    int call_internal(std::string const& method, msgpack::object const& params,
                      callback_t callback=callback_t());

};


template <typename ... Args>
int MsgpackRpc::call(std::string const& method, callback_t callback, Args const&... args) {
    msgpack::zone zone;
    msgpack::type::tuple<Args...> params(args...);
    return this->call_internal(method, msgpack::object(params, zone), callback);
}

template <typename ... Args>
int MsgpackRpc::call(std::string const& method, Args const&... args) {
    return this->call(method, callback_t(), args...);
}
