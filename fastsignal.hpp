#pragma once

#include <vector>
#include <functional>

namespace fastsignal {

namespace internal {
    struct Callback;
    struct Connection;
    class FastSignalBase;
} // namespace internal

class ConnectionView;
template<typename Signature>
class FastSignal;

namespace internal {

struct Callback
{
    void *obj = nullptr;
    void *fun = nullptr;
    Connection *conn = nullptr;
};

struct Connection
{
    FastSignalBase *sig = nullptr;
    int index = -1;
    int ref_count = 1;

    Connection(FastSignalBase *sig, int index) :
        sig(sig), index(index) {}

    Connection(const Connection &other) = delete;
    Connection &operator=(const Connection &other) = delete;
    Connection(Connection &&other) = delete;
    Connection &operator=(Connection &&other) = delete;

    void set_sig(FastSignalBase *sig) {
        this->sig = sig;
    }

    static void inc(Connection *conn) {
        if (!conn)
            return;
        ++conn->ref_count;
    }

    static void dec(Connection *conn) {
        if (!conn)
            return;
        --conn->ref_count;
        if (conn->ref_count == 0)
            delete conn;
    }

    inline void disconnect();
};

class FastSignalBase
{
protected:
    std::vector<Callback> callbacks;
    
    size_t callback_count = 0;
    bool is_dirty = false;

public:
    FastSignalBase() = default;
    FastSignalBase(const FastSignalBase &other) = delete;
    FastSignalBase &operator=(const FastSignalBase &other) = delete;

    FastSignalBase(FastSignalBase &&other) noexcept :
        callbacks(std::move(other.callbacks)), callback_count(other.callback_count) {
        other.callback_count = 0;
    }

    FastSignalBase &operator=(FastSignalBase &&other) noexcept {
        callbacks = std::move(other.callbacks);
        callback_count = other.callback_count;
        other.callback_count = 0;
        return *this;
    }

    virtual ~FastSignalBase() {
        for (auto &cb : callbacks) {
            if (!cb.conn)
                continue;
            cb.conn->set_sig(nullptr);
            Connection::dec(cb.conn);
            cb.conn = nullptr;
        }
    }

    void dirty(int index) {
        is_dirty = true;
        --callback_count;

        callbacks[index].fun = nullptr;
        callbacks[index].obj = nullptr;
    }

    size_t count() const {
        return callback_count;
    }
};

inline void Connection::disconnect()
{
    if (!sig)
        return;

    sig->dirty(index);
    index = -1;
    sig = nullptr;
}

} // namespace internal

class Disconnectable
{
    std::vector<internal::Connection*> connections;

    template<typename Signature>
    friend class FastSignal;
    void add_connection(internal::Connection *conn) {
        connections.emplace_back(conn);
        internal::Connection::inc(conn);
    }

public:
    virtual ~Disconnectable() {
        for (auto &conn : connections) {
            conn->disconnect();
            internal::Connection::dec(conn);
            conn = nullptr;
        }
    }
};

class ConnectionView
{
    internal::Connection *connection;

public:
    ConnectionView(internal::Connection *connection = nullptr) : connection(connection) {
        internal::Connection::inc(connection);
    }

    ~ConnectionView() {
        internal::Connection::dec(connection);
        connection = nullptr;
    }

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
        if (!connection)
            return;

        connection->disconnect();
        internal::Connection::dec(connection);
        connection = nullptr;
    }
};

template<typename RetType, typename... ArgTypes>
class FastSignal<RetType(ArgTypes...)> final : public internal::FastSignalBase
{
    using CallbackType = std::function<RetType(ArgTypes...)>;

public:
    template<auto fun, class ObjType>
    ConnectionView add(ObjType *obj) {
        static_assert(std::is_invocable_v<decltype(fun), ObjType*, ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        callbacks.push_back({reinterpret_cast<void*>(obj),
            reinterpret_cast<void*>(+[](void *obj, const ArgTypes&... args) -> RetType {
                (reinterpret_cast<ObjType*>(obj)->*fun)(args...);
            })});

        ++callback_count;
        internal::Connection *conn = new internal::Connection(this, callbacks.size() - 1);
        callbacks.back().conn = conn;
        if constexpr (std::is_base_of_v<Disconnectable, ObjType>)
            obj->add_connection(conn);

        return ConnectionView(conn);
    }

    ConnectionView add(RetType(fun)(ArgTypes...)) {
        static_assert(std::is_invocable_v<decltype(fun), ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        callbacks.push_back({nullptr, reinterpret_cast<void*>(fun)});

        ++callback_count;
        callbacks.back().conn = new internal::Connection(this, callbacks.size() - 1);
        return ConnectionView(callbacks.back().conn);
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

        if (!is_dirty)
            return;

        size_t size = 0;
        for (size_t i = 0; i < callbacks.size(); i++) {
            if (callbacks[i].fun == nullptr) {
                callbacks[i].conn->set_sig(nullptr);
                internal::Connection::dec(callbacks[i].conn);
                callbacks[i].conn = nullptr;
            } else {
                callbacks[size] = callbacks[i];
                callbacks[size].conn->index = size;
                size++;
            }
        }

        is_dirty = false;
        callbacks.resize(size);
    }
};

} // namespace fastsignal
