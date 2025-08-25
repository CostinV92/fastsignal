#include <iostream>

#include "simplesignal.hpp"

int main()
{
    Baz baz0, baz1;

    SimpleSignal<void(Parm&)> sig;
    auto con1 = sig.add<&Baz::fun_Parm>(&baz0);
    auto con2 = sig.add<&Baz::fun_Parm>(&baz0);

    con1.disconnect();

    Parm p;
    sig(p);

    std::cout << "====================\n";
}