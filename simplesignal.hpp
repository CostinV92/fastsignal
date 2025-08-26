#pragma once

#include <vector>
#include <functional>

namespace simplesignal {

struct Connection;

struct Callback
{
    void *fun = nullptr;
    void *obj = nullptr;

    Connection *conn = nullptr;

    bool operator==(const Callback &other) const {
        return fun == other.fun && obj == other.obj;
    }
};

using CallbacksContainer = std::vector<Callback>;

struct SimpleSignalBase
{
    virtual ~SimpleSignalBase() = default;

    virtual void dirty(int index) = 0;
};

template<typename Signature>
class SimpleSignal;

struct Connection
{
    SimpleSignalBase *sig;
    int index;

    Connection(SimpleSignalBase *sig, int index) :
        sig(sig), index(index) {}

    Connection(const Connection &other) = delete;
    Connection &operator=(const Connection &other) = delete;

    Connection(Connection &&other) noexcept :
        sig(other.sig), index(other.index) {
        other.sig = nullptr;
        other.index = -1;
    }

    Connection &operator=(Connection &&other) noexcept {
        sig = other.sig;
        index = other.index;
        other.sig = nullptr;
        other.index = -1;
        return *this;
    }

    void disconnect() {
        sig->dirty(index);
    }
};

struct ConnectionView
{
    Connection *connection;

    ConnectionView(Connection *connection) : connection(connection) {}

    ConnectionView(const ConnectionView &other) = delete;
    ConnectionView &operator=(const ConnectionView &other) = delete;

    ConnectionView(ConnectionView &&other) noexcept : connection(other.connection) {
        other.connection = nullptr;
    }

    ConnectionView &operator=(ConnectionView &&other) noexcept {
        connection = other.connection;
        other.connection = nullptr;
        return *this;
    }

    void disconnect() {
        connection->disconnect();
        connection = nullptr;
    }
};

template<typename RetType, typename... ArgTypes>
class SimpleSignal<RetType(ArgTypes...)> : public SimpleSignalBase
{
    using CallbackType = std::function<RetType(ArgTypes...)>;
    
    CallbacksContainer callbacks;

    bool isDirty = false;
public:

    SimpleSignal() = default;
    SimpleSignal(const SimpleSignal &other) = delete;
    SimpleSignal &operator=(const SimpleSignal &other) = delete;

    SimpleSignal(SimpleSignal &&other) noexcept :
        callbacks(std::move(other.callbacks)) {}
    SimpleSignal &operator=(SimpleSignal &&other) noexcept {
        callbacks = std::move(other.callbacks);
        return *this;
    }

    ~SimpleSignal() {
        for (auto &cb : callbacks)
            if (cb.conn) {
                delete cb.conn;
                cb.conn = nullptr;
            }
    }

    void dirty(int index) override {
        isDirty = true;
        callbacks[index].conn->index = -1;
        callbacks[index].fun = nullptr;
        callbacks[index].obj = nullptr;
    }

    template<auto fun, class ObjType>
    ConnectionView add(ObjType *obj) {
        static_assert(std::is_invocable_v<decltype(fun), ObjType*, ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        auto &cb = callbacks.emplace_back();

        cb.obj = reinterpret_cast<void*>(obj);
        cb.fun = reinterpret_cast<void*>(+[](void *obj, const ArgTypes&... args) -> RetType {
            (reinterpret_cast<ObjType*>(obj)->*fun)(args...);
        });

        cb.conn = new Connection(this, callbacks.size() - 1);
        return ConnectionView(cb.conn);
    }

    ConnectionView add(RetType(fun)(ArgTypes...)) {
        static_assert(std::is_invocable_v<decltype(fun), ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        auto &cb = callbacks.emplace_back();

        cb.obj = nullptr;
        cb.fun = reinterpret_cast<void*>(fun);

        cb.conn = new Connection(this, callbacks.size() - 1);
        return ConnectionView(cb.conn);
    }

    // TODO(victor);
    // void add(CallbackType fun) {
    //     (void)fun;
    // }

    template<typename... ActualArgs>
    void operator()(ActualArgs&&... args) {
        // TODO(victor) - check if the parameters match the signature of the callback
        for (auto &cb : callbacks) {
            if (cb.fun == nullptr)
                continue;

            if (cb.obj)
                reinterpret_cast<RetType(*)(void*, const ArgTypes&...)>(cb.fun)(cb.obj, std::forward<ActualArgs>(args)...);
            else
                reinterpret_cast<RetType(*)(ArgTypes...)>(cb.fun)(std::forward<ActualArgs>(args)...);
        }

        if (isDirty) {
            isDirty = false;
            size_t size = 0;
            for (size_t i = 0; i < callbacks.size(); i++) {
                if (callbacks[i].conn->index == -1) {
                    delete callbacks[i].conn;
                    callbacks[i].conn = nullptr;
                    continue;
                }

                callbacks[size] = callbacks[i];
                callbacks[size].conn->index = size;
                size++;
            }

            callbacks.resize(size);
        }
    }
};

} // namespace simplesignal