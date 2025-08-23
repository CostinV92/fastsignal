#pragma once

#include <vector>
#include <functional>

namespace simplesignal {

template<typename Signature>
class SimpleSignal;

template<typename RetType, typename... ArgTypes>
class SimpleSignal<RetType(ArgTypes...)>
{
    uint64_t next_id = 0;

    using CallbackType = std::function<RetType(ArgTypes...)>;

    struct ConnectionData {
        uint64_t id;
        CallbackType callback;

        ConnectionData(uint64_t id, CallbackType&& callback) :
            id(id), callback(std::forward<CallbackType>(callback)) {}
    };

    using Container = std::vector<ConnectionData>;
    Container callbacks;

public:
    class Connection {
        SimpleSignal<RetType(ArgTypes...)>* signal;
        uint64_t id;
    
    public:
        Connection(uint64_t id, SimpleSignal<RetType(ArgTypes...)>* signal) :
            id(id), signal(signal) {}

        void disconnect() {
            if (signal) {
                for (auto it = signal->callbacks.begin(); it != signal->callbacks.end(); ++it) {
                    if (it->id == id) {
                        signal->callbacks.erase(it);
                        break;
                    }
                }
            }
            signal = nullptr;
        }

        bool is_connected() const {
            return signal != nullptr;
        }
    };

    Connection add(CallbackType&& callback) {
        callbacks.emplace_back(next_id, std::forward<CallbackType>(callback));
        return Connection(next_id++, this);
    }

    void operator()(ArgTypes&&... args) {
        emit(std::forward<ArgTypes>(args)...);
    }

    void emit(ArgTypes&&... args) {
        for (auto& callback_data : callbacks) {
            callback_data.callback(std::forward<ArgTypes>(args)...);
        }
    }
};

} // namespace simplesignal