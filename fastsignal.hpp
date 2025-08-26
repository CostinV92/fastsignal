#pragma once

#include <vector>
#include <functional>

namespace fastsignal {

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

struct FastSignalBase
{
    virtual ~FastSignalBase() = default;

    virtual void dirty(int index) = 0;
};

template<typename Signature>
class FastSignal;

struct Connection
{
    FastSignalBase *sig;
    int index;

    Connection(FastSignalBase *sig, int index) :
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

    ConnectionView(Connection *connection = nullptr) : connection(connection) {}

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
        if (connection) {
            connection->disconnect();
            connection = nullptr;
        }
    }
};

template<typename RetType, typename... ArgTypes>
class FastSignal<RetType(ArgTypes...)> : public FastSignalBase
{
    using CallbackType = std::function<RetType(ArgTypes...)>;
    
    CallbacksContainer callbacks;

    bool isDirty = false;
    size_t m_size = 0;
public:

    FastSignal() = default;
    FastSignal(const FastSignal &other) = delete;
    FastSignal &operator=(const FastSignal &other) = delete;

    FastSignal(FastSignal &&other) noexcept :
        callbacks(std::move(other.callbacks)) {}
    FastSignal &operator=(FastSignal &&other) noexcept {
        callbacks = std::move(other.callbacks);
        return *this;
    }

    ~FastSignal() {
        for (auto &cb : callbacks)
            if (cb.conn) {
                delete cb.conn;
                cb.conn = nullptr;
            }
    }

    void dirty(int index) override {
        isDirty = true;
        --m_size;
        callbacks[index].conn->index = -1;
        callbacks[index].fun = nullptr;
        callbacks[index].obj = nullptr;
    }

    size_t size() const {
        return m_size;
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

        ++m_size;
        cb.conn = new Connection(this, callbacks.size() - 1);
        return ConnectionView(cb.conn);
    }

    ConnectionView add(RetType(fun)(ArgTypes...)) {
        static_assert(std::is_invocable_v<decltype(fun), ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        auto &cb = callbacks.emplace_back();

        cb.obj = nullptr;
        cb.fun = reinterpret_cast<void*>(fun);

        ++m_size;
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

} // namespace fastsignal