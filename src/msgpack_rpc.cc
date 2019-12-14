#include "./msgpack_rpc.h"

#include <cassert>
#include <QtDebug>

#include <iostream>


class QIODeviceWrapper {
    QIODevice* iodevice_;
public:
    QIODeviceWrapper(QIODevice* iodevice): iodevice_(iodevice) {}
    void write(const char* buf, size_t len) {
        int res = iodevice_->write(buf, len);
        if (res != len)
            qFatal("Write failed: %d/%d", res, (int)len);
    }
};


MsgpackRpc::MsgpackRpc(std::unique_ptr<QIODevice> iodevice):
iodevice_(std::move(iodevice)) {
    iodevice_->open(QIODevice::ReadWrite);

    connect(iodevice_.get(), &QIODevice::readyRead,
            this, &MsgpackRpc::do_read);
    connect(iodevice_.get(), &QIODevice::aboutToClose,
            this, &MsgpackRpc::on_close);
}

bool MsgpackRpc::is_open() {
    return iodevice_->isOpen();
}


int MsgpackRpc::call_internal(std::string const& method, msgpack::object const& params,
                              callback_t callback) {
    QIODeviceWrapper iodevice_wrapper(this->iodevice_.get());

    uint32_t id = counter_++;

    qDebug() << "Sending msgpack request" << id << method.c_str();
    msgpack::type::tuple<int, uint32_t, std::string, msgpack::object>
        request_body(0, id, method, params);
    msgpack::pack(iodevice_wrapper, request_body);

    callbacks_[id] = std::move(callback);
    return 0;  // TODO: return error?
}

void MsgpackRpc::do_read() {
    auto bytes_available = iodevice_->bytesAvailable();
    assert(bytes_available > 0);

    unpacker_.reserve_buffer(bytes_available);
    auto read_ret = iodevice_->read(unpacker_.buffer(), bytes_available);
    assert(read_ret == bytes_available);
    unpacker_.buffer_consumed(read_ret);

    msgpack::object_handle result;
    while (unpacker_.next(result)) {
        msgpack::object obj = result.get();

        if (obj.type != msgpack::type::ARRAY) {
            qWarning() << "Invalid msgpack message type " << obj.type;
            continue;
        }

        if (obj.via.array.size == 4 &&
            obj.via.array.ptr[0].type == msgpack::type::POSITIVE_INTEGER &&
            obj.via.array.ptr[0].via.i64 == 1) {
            // response
            msgpack::type::tuple<int, uint32_t, msgpack::object, msgpack::object> response_body;
            try {
                obj.convert(response_body);
            } catch (...) {
                qWarning() << "Unable to convert msgpack response";
                continue;
            }
            assert(response_body.get<0>() == 1);
            uint32_t msgid = response_body.get<1>();
            qDebug() << "Received msgpack response" << msgid;

            auto callback_it = callbacks_.find(msgid);
            if (callback_it == callbacks_.end()) {
                qWarning() << "Unable to find callback";
                continue;
            }

            auto callback = callback_it->second;
            callbacks_.erase(callback_it);
            if (callback)
                callback(response_body.get<2>().is_nil() ? 0 : -1,
                         response_body.get<2>().is_nil() ? response_body.get<3>() : response_body.get<2>());
        } else if (obj.via.array.size == 3 &&
            obj.via.array.ptr[0].type == msgpack::type::POSITIVE_INTEGER &&
            obj.via.array.ptr[0].via.i64 == 2) {
            // notification
            msgpack::type::tuple<int, std::string, msgpack::object> notification_body;
            try {
                obj.convert(notification_body);
            } catch (...) {
                qWarning() << "Unable to convert msgpack notification";
                continue;
            }
            assert(notification_body.get<0>() == 2);

            qDebug() << "Received msgpack notification" << notification_body.get<1>().c_str();
            // std::cerr << notification_body.get<2>() << std::endl;
            emit on_notification(notification_body.get<1>(),
                                 notification_body.get<2>());
        } else {
            qWarning() << "Invalid msgpack array size" << obj.via.array.size;
        }
    }
}
