#pragma once

#include <vector>
#include <functional>

namespace fastsignal {

namespace internal {
    struct Callback;
    struct Connection;
    class FastSignalBase;
} // namespace internal

class Disconnectable;
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
    bool is_disconnectable = false;

    Connection(FastSignalBase *sig, int index, bool is_disconnectable) :
        sig(sig), index(index), is_disconnectable(is_disconnectable) {}

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

} // namespace internal

class Disconnectable
{
    std::vector<internal::Connection*> connections;

    template<typename Signature>
    friend class FastSignal;
    friend class internal::FastSignalBase;

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

namespace internal {

class FastSignalBase
{
protected:
    mutable std::vector<Callback> callbacks;
    
    size_t callback_count = 0;
    mutable bool is_dirty = false;

public:
    FastSignalBase() = default;

    FastSignalBase(const FastSignalBase &other) : callbacks(other.callbacks),
        callback_count(other.callback_count), is_dirty(other.is_dirty) {
        for (auto &cb : callbacks) {
            if (!cb.conn)
                continue;

            cb.conn = new internal::Connection(this, cb.conn->index, other.callbacks[cb.conn->index].conn->is_disconnectable);
            if (other.callbacks[cb.conn->index].conn->is_disconnectable)
                reinterpret_cast<Disconnectable*>(cb.obj)->add_connection(cb.conn);
        }
    }

    FastSignalBase& operator=(const FastSignalBase &other) {
        callbacks = other.callbacks;
        callback_count = other.callback_count;
        is_dirty = other.is_dirty;

        for (auto &cb : callbacks) {
            if (!cb.conn)
                continue;

            cb.conn = new internal::Connection(this, cb.conn->index, other.callbacks[cb.conn->index].conn->is_disconnectable);
            if (other.callbacks[cb.conn->index].conn->is_disconnectable)
                reinterpret_cast<Disconnectable*>(cb.obj)->add_connection(cb.conn);
        }

        return *this;
    }

    FastSignalBase(FastSignalBase &&other) noexcept :
        callbacks(std::move(other.callbacks)), callback_count(other.callback_count),
            is_dirty(other.is_dirty) {
        other.callback_count = 0;

        for (auto &cb : callbacks) {
            if (!cb.conn)
                continue;
            cb.conn->set_sig(this);
        }
    }

    FastSignalBase& operator=(FastSignalBase &&other) noexcept {
        callbacks = std::move(other.callbacks);
        callback_count = other.callback_count;
        is_dirty = other.is_dirty;
        other.callback_count = 0;

        for (auto &cb : callbacks) {
            if (!cb.conn)
                continue;
            cb.conn->set_sig(this);
        }

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
    sig = nullptr;
}

} // namespace internal

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

    ConnectionView(const ConnectionView &other) {
        connection = other.connection;
        internal::Connection::inc(connection);
    }

    ConnectionView& operator=(const ConnectionView &other) {
        connection = other.connection;
        internal::Connection::inc(connection);
        return *this;
    }

    ConnectionView(ConnectionView &&other) noexcept : connection(other.connection) {
        other.connection = nullptr;
    }

    ConnectionView& operator=(ConnectionView &&other) noexcept {
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
        using FunType = decltype(fun);
        static_assert(std::is_invocable_v<FunType, ObjType*, ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");
        static_assert(std::is_same_v<std::invoke_result_t<FunType, ObjType*, ArgTypes...>, RetType>,
            "Callback must return the signal's declared return type");

        constexpr bool is_disconnectable = std::is_base_of_v<Disconnectable, ObjType>;

        internal::Connection *conn = new internal::Connection(this, callbacks.size(), is_disconnectable);
        callbacks.push_back({reinterpret_cast<void*>(obj),
            reinterpret_cast<void*>(+[](void *obj, const ArgTypes&... args) -> RetType {
                (reinterpret_cast<ObjType*>(obj)->*fun)(args...);
            }), conn});
        ++callback_count;

        if constexpr (is_disconnectable)
            obj->add_connection(conn);

        return ConnectionView(conn);
    }

    ConnectionView add(RetType(fun)(ArgTypes...)) {
        internal::Connection *conn = new internal::Connection(this, callbacks.size(), false);
        callbacks.push_back({nullptr, reinterpret_cast<void*>(fun), conn});
        ++callback_count;
        return ConnectionView(conn);
    }

    // TODO(victor);
    // void add(CallbackType fun) {
    //     (void)fun;
    // }

    template<typename... ActualArgs>
    void operator()(ActualArgs&&... args) const {
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

#ifdef FASTSIGNAL_TEST
    size_t actual_count() const {
        return callbacks.size();
    }
#endif
};

} // namespace fastsignal
