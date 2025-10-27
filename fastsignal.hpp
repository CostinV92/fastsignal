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
    std::shared_ptr<Connection> conn = nullptr;
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

    inline void disconnect();
    inline void update_sig_obj(Disconnectable *obj);
};

class FastSignalBase
{
protected:
    mutable std::vector<Callback> callbacks;
    
    size_t callback_count = 0;
    mutable bool is_dirty = false;

public:
    FastSignalBase() = default;

    FastSignalBase(const FastSignalBase&) {}
    FastSignalBase& operator=(const FastSignalBase&) { return *this; }

    FastSignalBase(FastSignalBase &&other) :
        callbacks(std::move(other.callbacks)), callback_count(other.callback_count),
            is_dirty(other.is_dirty) {
        other.callback_count = 0;

        for (auto &cb : callbacks) {
            if (!cb.conn)
                continue;
            cb.conn->set_sig(this);
        }
    }

    FastSignalBase& operator=(FastSignalBase &&other) {
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
            cb.conn = nullptr;
        }
    }

    void update_sig_obj(int index, Disconnectable *obj) {
        callbacks[index].obj = obj;
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

inline void Connection::update_sig_obj(Disconnectable *obj) {
    if (!sig)
        return;
    sig->update_sig_obj(index, obj);
}

} // namespace internal

class Disconnectable
{
    std::vector<std::weak_ptr<internal::Connection>> connections;

    template<typename Signature>
    friend class FastSignal;
    friend class internal::FastSignalBase;

    void add_connection(std::shared_ptr<internal::Connection> conn) {
        connections.emplace_back(conn);
    }

public:

    Disconnectable() = default;

    Disconnectable(const Disconnectable&) {};
    Disconnectable& operator=(const Disconnectable&) { return *this; };

    Disconnectable(Disconnectable &&other) : connections(std::move(other.connections)) {
        for (auto &conn : connections) {
            std::shared_ptr<internal::Connection> sp = conn.lock();
            if (!sp)
                continue;
            sp->update_sig_obj(this);
        }
    };
    Disconnectable& operator=(Disconnectable &&other) {
        connections = std::move(other.connections);
        for (auto &conn : connections) {
            std::shared_ptr<internal::Connection> sp = conn.lock();
            if (!sp)
                continue;
            sp->update_sig_obj(this);
        }
        return *this;
    };

    virtual ~Disconnectable() {
        for (auto &conn : connections) {
            std::shared_ptr<internal::Connection> sp = conn.lock();
            if (!sp)
                continue;
            sp->disconnect();
            conn.reset();
        }
    }
};

class ConnectionView
{
    std::weak_ptr<internal::Connection> connection;

public:
    ConnectionView(std::shared_ptr<internal::Connection> connection = nullptr) : connection(connection) {}

    ~ConnectionView() {
        connection.reset();
    }

    ConnectionView(const ConnectionView &other) {
        connection = other.connection;
    }

    ConnectionView& operator=(const ConnectionView &other) {
        connection = other.connection;
        return *this;
    }

    ConnectionView(ConnectionView &&other) : connection(other.connection) {
        other.connection.reset();
    }

    ConnectionView& operator=(ConnectionView &&other) {
        connection = other.connection;
        other.connection.reset();
        return *this;
    }

    void disconnect() {
        std::shared_ptr<internal::Connection> conn = connection.lock();
        if (!conn)
            return;

        conn->disconnect();
        connection.reset();
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

        std::shared_ptr<internal::Connection> conn = std::make_shared<internal::Connection>(this, callbacks.size(), is_disconnectable);
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
        std::shared_ptr<internal::Connection> conn = std::make_shared<internal::Connection>(this, callbacks.size(), false);
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
