#pragma once

#include <vector>
#include <functional>

namespace simplesignal_p {

struct Callback
{
    void *fun = nullptr;
    void *obj = nullptr;

    bool operator==(const Callback &other) const {
        return fun == other.fun && obj == other.obj;
    }
};

using CallbacksContainer = std::vector<Callback>;

struct Connection;
using ConnectionsContainer = std::vector<Connection*>;

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
    ConnectionsContainer connections;

    bool isDirty = false;
public:

    SimpleSignal() = default;
    SimpleSignal(const SimpleSignal &other) = delete;
    SimpleSignal &operator=(const SimpleSignal &other) = delete;

    SimpleSignal(SimpleSignal &&other) noexcept :
        callbacks(std::move(other.callbacks)), connections(std::move(other.connections)) {}
    SimpleSignal &operator=(SimpleSignal &&other) noexcept {
        callbacks = std::move(other.callbacks);
        connections = std::move(other.connections);
        return *this;
    }

    ~SimpleSignal() {
        for (auto &connection : connections)
            delete connection;
    }

    void dirty(int index) override {
        isDirty = true;
        connections[index]->index = -1;
        callbacks[index].fun = nullptr;
        callbacks[index].obj = nullptr;
    }
    
    template<auto fun, class ObjType>
    ConnectionView add(ObjType *obj) {
        static_assert(std::is_invocable_v<decltype(fun), ObjType*, ArgTypes...>,
            "Callback must be invocable with the signal's declared parameters");

        auto &cb = callbacks.emplace_back();

        cb.obj = reinterpret_cast<void*>(obj);
        cb.fun = reinterpret_cast<void*>(+[](void *obj, ArgTypes... args) -> RetType {
            (reinterpret_cast<ObjType*>(obj)->*fun)(args...);
        });

        connections.emplace_back(new Connection(this, callbacks.size() - 1));
        return ConnectionView(connections.back());
    }

    ConnectionView add(RetType(fun)(ArgTypes...)) {
        auto &cb = callbacks.emplace_back();

        cb.obj = nullptr;
        cb.fun = reinterpret_cast<void*>(fun);

        connections.emplace_back(new Connection(this, callbacks.size() - 1));
        return ConnectionView(connections.back());
    }

    void add(CallbackType fun) {
        // TODO(victor);
        (void)fun;
    }

    template<typename... ActualArgs>
    void operator()(ActualArgs&&... args) {
        // TODO(victor) - check if the parameters match the signature of the callback
        for (auto &cb : callbacks) {
            if (cb.fun == nullptr)
                continue;

            if (cb.obj)
                reinterpret_cast<RetType(*)(void*, ArgTypes&...)>(cb.fun)(cb.obj, args...);
            else
                reinterpret_cast<RetType(*)(ActualArgs&...)>(cb.fun)(args...);
        }

        if (isDirty) {
            isDirty = false;
            int size = 0;
            for (int i = 0; i < connections.size(); i++) {
                if (connections[i]->index == -1) {
                    delete connections[i];
                    continue;
                }

                callbacks[size] = callbacks[i];
                connections[size] = connections[i];
                connections[size]->index = size++;
            }

            callbacks.resize(size);
            connections.resize(size);
        }
    }
};

} // namespace simplesignal