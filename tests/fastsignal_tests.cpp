#include <array>

#include <gtest/gtest.h>
#include <gmock/gmock.h> 

#include "fastsignal.hpp"

using namespace fastsignal;

int global_value1 = 0;
int global_value2 = 0;

void set_global_value1(int x)
{
    global_value1 = x;
}

void set_global_value2(int x)
{
    global_value2 = x;
}

struct GlobalParam {
    int value;
    GlobalParam(int x) : value(x) {}

    friend bool operator==(const GlobalParam& lhs, const GlobalParam& rhs)
    {
        return lhs.value == rhs.value;
    }
};

GlobalParam global_param1(0);
GlobalParam global_param2(0);

void set_global_param1(GlobalParam x)
{
    global_param1 = x;
}

void set_global_param2(GlobalParam x)
{
    global_param2 = x;
}

struct Foo {
    MOCK_METHOD(void, set_value, (int x));
    MOCK_METHOD(void, set_param, (GlobalParam x));
    MOCK_METHOD(void, set_param_ref, (GlobalParam& x));
    MOCK_METHOD(void, set_param_const, (const GlobalParam& x));
};

class FastSignalTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        global_value1 = 0;
        global_value2 = 0;

        global_param1.value = 0;
        global_param2.value = 0;
    }
};

TEST_F(FastSignalTest, test_signal_empty)
{
    FastSignal<void(int)> sig;
    EXPECT_EQ(sig.size(), 0);

    sig(1);
    EXPECT_EQ(global_value1, 0);
    EXPECT_EQ(global_value2, 0);
}

TEST_F(FastSignalTest, test_signal_multiple_disconnect)
{
    FastSignal<void(int)> sig;
    auto con1 = sig.add(set_global_value1);
    EXPECT_EQ(sig.size(), 1);
    con1.disconnect();
    EXPECT_EQ(sig.size(), 0);
    con1.disconnect();
    EXPECT_EQ(sig.size(), 0);
    con1.disconnect();
    EXPECT_EQ(sig.size(), 0);
    con1.disconnect();
    EXPECT_EQ(sig.size(), 0);
}

TEST_F(FastSignalTest, test_signal_size)
{
    FastSignal<void(int)> sig;

    int size = 0;
    std::array<ConnectionView, 100> connections;
    for (auto& con : connections) {
        con = sig.add(set_global_value1);
        EXPECT_EQ(sig.size(), ++size);
    }
    for (auto& con : connections) {
        con.disconnect();
        EXPECT_EQ(sig.size(), --size);
    }
    EXPECT_EQ(sig.size(), 0);
}

TEST_F(FastSignalTest, test_signal_call)
{
    FastSignal<void(int)> sig;
    sig.add(set_global_value1);
    sig(1);
    EXPECT_EQ(global_value1, 1);

    sig(2);
    EXPECT_EQ(global_value1, 2);

    sig.add(set_global_value2);
    sig(3);
    EXPECT_EQ(global_value1, 3);
    EXPECT_EQ(global_value2, 3);
}

TEST_F(FastSignalTest, test_signal_disconnect)
{
    FastSignal<void(int)> sig;
    auto con1 = sig.add(set_global_value1);
    auto con2 = sig.add(set_global_value2);
    sig(1);
    EXPECT_EQ(global_value1, 1);
    EXPECT_EQ(global_value2, 1);

    con1.disconnect();
    sig(2);
    EXPECT_EQ(global_value1, 1);
    EXPECT_EQ(global_value2, 2);

    con2.disconnect();
    sig(3);
    EXPECT_EQ(global_value1, 1);
    EXPECT_EQ(global_value2, 2);
}

TEST_F(FastSignalTest, test_signal_param)
{
    FastSignal<void(GlobalParam)> sig;
    auto con1 = sig.add(set_global_param1);
    auto con2 = sig.add(set_global_param2);
    sig(GlobalParam(1));
    EXPECT_EQ(global_param1.value, 1);
    EXPECT_EQ(global_param2.value, 1);

    sig(GlobalParam(2));
    EXPECT_EQ(global_param1.value, 2);
    EXPECT_EQ(global_param2.value, 2);

    sig(GlobalParam(3));
    EXPECT_EQ(global_param1.value, 3);
    EXPECT_EQ(global_param2.value, 3);

    con1.disconnect();
    sig(GlobalParam(4));
    EXPECT_EQ(global_param1.value, 3);
    EXPECT_EQ(global_param2.value, 4);

    con2.disconnect();
    sig(GlobalParam(5));
    EXPECT_EQ(global_param1.value, 3);
    EXPECT_EQ(global_param2.value, 4);
}

TEST_F(FastSignalTest, test_signal_member_function)
{
    FastSignal<void(int)> sig;
    Foo foo;
    auto con = sig.add<&Foo::set_value>(&foo);
    EXPECT_CALL(foo, set_value(1));
    sig(1);

    EXPECT_CALL(foo, set_value(2));
    sig(2);

    con.disconnect();
    EXPECT_CALL(foo, set_value(3)).Times(0);
    sig(3);
}

TEST_F(FastSignalTest, test_signal_member_function_param)
{
    FastSignal<void(GlobalParam)> sig;
    Foo foo;
    auto con = sig.add<&Foo::set_param>(&foo);
    EXPECT_CALL(foo, set_param(global_param1));
    sig(global_param1);

    EXPECT_CALL(foo, set_param(global_param2));
    sig(global_param2);

    con.disconnect();
    EXPECT_CALL(foo, set_param(global_param1)).Times(0);
    sig(global_param1);
}

TEST_F(FastSignalTest, test_signal_member_function_param_ref)
{
    FastSignal<void(GlobalParam&)> sig;
    Foo foo;
    sig.add<&Foo::set_param_ref>(&foo);
    EXPECT_CALL(foo, set_param_ref(global_param1));
    sig(global_param1);

    // This should not compile
    // Binding const lvalue or rvalue to a non-const reference is not allowed
    //
    //const GlobalParam param(2);
    //sig(param);
    //
    //sig(GlobalParam(2));
}

TEST_F(FastSignalTest, test_signal_member_function_param_const)
{
    FastSignal<void(const GlobalParam&)> sig;
    Foo foo;

    // Calling using const parameter (lvalue) because cb accepts const parameter
    const GlobalParam param(1);
    sig.add<&Foo::set_param_const>(&foo);
    EXPECT_CALL(foo, set_param_const(param));
    sig(param);

    // Calling using rvalue because cb accepts const parameter
    EXPECT_CALL(foo, set_param_const(GlobalParam(2)));
    sig(GlobalParam(2));
}

TEST_F(FastSignalTest, test_signal_connection_view_move)
{
    FastSignal<void(int)> sig;
    auto con1 = sig.add(set_global_value1);
    EXPECT_EQ(sig.size(), 1);

    ConnectionView con2;

    // This should not compile
    // Copy assignment operator is deleted
    // con2 = con1;

    con2 = std::move(con1);
    EXPECT_EQ(con1.connection, nullptr);
    EXPECT_EQ(sig.size(), 1);

    con1.disconnect();
    sig(5);
    EXPECT_EQ(global_value1, 5);

    con2.disconnect();
    sig(6);
    EXPECT_EQ(global_value1, 5);
    EXPECT_EQ(sig.size(), 0);
}

TEST_F(FastSignalTest, test_signal_move)
{
    FastSignal<void(int)> sig1;
    sig1.add(set_global_value1);
    FastSignal<void(int)> sig2(std::move(sig1));

    EXPECT_EQ(sig1.size(), 0);
    EXPECT_EQ(sig2.size(), 1);

    // This should not fail, but no callback should be called
    sig1(1);
    EXPECT_EQ(global_value1, 0);

    sig2(2);
    EXPECT_EQ(global_value1, 2);

    FastSignal<void(int)> sig3;
    sig3(3);
    EXPECT_EQ(global_value1, 2);

    sig3 = std::move(sig2);
    EXPECT_EQ(sig2.size(), 0);
    EXPECT_EQ(sig3.size(), 1);

    sig3(4);
    EXPECT_EQ(global_value1, 4);
}

TEST_F(FastSignalTest, test_signal_connection_lifetime)
{
    {
        // Connection outlives the signal
        // Calling disconnect should not crash
        ConnectionView con1;
        Foo observer;
        {
            FastSignal<void(int)> sig1;
            con1 = sig1.add<&Foo::set_value>(&observer);
            EXPECT_CALL(observer, set_value(1));
            sig1(1);
        }
        con1.disconnect();
    }

    {
        // Signal outlives the connection
        // signaling should work, callback should be called
        FastSignal<void(int)> sig1;
        Foo observer;
        {
            auto con2 = sig1.add<&Foo::set_value>(&observer);
            EXPECT_CALL(observer, set_value(1));
            sig1(1);
        }
        EXPECT_CALL(observer, set_value(2));
        sig1(2);
    }
}
