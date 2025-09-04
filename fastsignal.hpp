#pragma once

#include <vector>
#include <functional>

namespace fastsignal {

struct Callback;
struct Connection;
struct ConnectionView;
struct FastSignalBase;
template<typename Signature>
class FastSignal;

struct Callback
{
    void *obj = nullptr;
    void *fun = nullptr;

    Callback(void *obj = nullptr, void *fun = nullptr) :
        obj(obj), fun(fun) {}

    bool operator==(const Callback &other) const {
        return fun == other.fun && obj == other.obj;
    }
};

using CallbacksContainer = std::vector<Callback>;

struct Connection
{
    FastSignalBase *sig;
    int index;
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

struct ConnectionView
{
    Connection *connection;

    ConnectionView(Connection *connection = nullptr) : connection(connection) {
        Connection::inc(connection);
    }

    ~ConnectionView() {
        Connection::dec(connection);
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
        Connection::dec(connection);
        connection = nullptr;
    }
};

struct FastSignalBase
{
    CallbacksContainer m_callbacks;
    std::vector<Connection*> m_connections;
    
    size_t m_size = 0;
    bool m_dirty = false;

    FastSignalBase() = default;
    FastSignalBase(const FastSignalBase &other) = delete;
    FastSignalBase &operator=(const FastSignalBase &other) = delete;

    FastSignalBase(FastSignalBase &&other) noexcept :
        m_callbacks(std::move(other.m_callbacks)), m_size(other.m_size) {
        other.m_size = 0;
    }

    FastSignalBase &operator=(FastSignalBase &&other) noexcept {
        m_callbacks = std::move(other.m_callbacks);
        m_size = other.m_size;
        other.m_size = 0;
        return *this;
    }

    virtual ~FastSignalBase() {
        for (auto &conn : m_connections) {
            if (!conn)
                continue;
            conn->set_sig(nullptr);
            Connection::dec(conn);
            conn = nullptr;
        }
    }

    void dirty(int index) {
        m_dirty = true;
        --m_size;
        m_connections[index]->index = -1;
        m_callbacks[index].fun = nullptr;
        m_callbacks[index].obj = nullptr;
    }

    size_t size() const {
        return m_size;
    }
};

template<typename RetType, typename... ArgTypes>
class FastSignal<RetType(ArgTypes...)> final : public FastSignalBase
{
    using CallbackType = std::function<RetType(ArgTypes...)>;

public:

    template<auto fun, class ObjType>
    ConnectionView add(ObjType *obj) {
        static_assert(std::is_invocable_v<decltype(fun), ObjType*, ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        m_callbacks.emplace_back(reinterpret_cast<void*>(obj),
            reinterpret_cast<void*>(+[](void *obj, const ArgTypes&... args) -> RetType {
                (reinterpret_cast<ObjType*>(obj)->*fun)(args...);
            }));

        ++m_size;
        m_connections.emplace_back(new Connection(this, m_callbacks.size() - 1));
        return ConnectionView(m_connections.back());
    }

    ConnectionView add(RetType(fun)(ArgTypes...)) {
        static_assert(std::is_invocable_v<decltype(fun), ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        m_callbacks.emplace_back(nullptr, reinterpret_cast<void*>(fun));

        ++m_size;
        m_connections.emplace_back(new Connection(this, m_callbacks.size() - 1));
        return ConnectionView(m_connections.back());
    }

    // TODO(victor);
    // void add(CallbackType fun) {
    //     (void)fun;
    // }

    template<typename... ActualArgs>
    void operator()(ActualArgs&&... args) {
        // TODO(victor) - check if the parameters match the signature of the callback
        for (auto &cb : m_callbacks) {
            if (cb.fun == nullptr)
                continue;

            if (cb.obj)
                reinterpret_cast<RetType(*)(void*, const ArgTypes&...)>(cb.fun)(cb.obj, std::forward<ActualArgs>(args)...);
            else
                reinterpret_cast<RetType(*)(ArgTypes...)>(cb.fun)(std::forward<ActualArgs>(args)...);
        }

        if (!m_dirty)
            return;

        size_t size = 0;
        for (size_t i = 0; i < m_callbacks.size(); i++) {
            if (m_connections[i]->index == -1) {
                m_connections[i]->set_sig(nullptr);
                Connection::dec(m_connections[i]);
                m_connections[i] = nullptr;
            } else {
                m_callbacks[size] = m_callbacks[i];
                m_connections[size] = m_connections[i];
                m_connections[size]->index = size;
                size++;
            }
        }

        m_dirty = false;
        m_callbacks.resize(size);
        m_connections.resize(size);
    }
};

inline void Connection::disconnect() {
    if (sig)
        sig->dirty(index);
}

} // namespace fastsignal
