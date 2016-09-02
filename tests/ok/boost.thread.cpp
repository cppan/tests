/*
local_settings:
    build:
        cxx_flags: -std=c++11

dependencies:
  pvt.cppan.demo.boost.thread: 1.61.0
*/

#include <iostream>
#include <fstream>
#include <string>

#include <boost/thread.hpp>

int main(int argc, char* argv[])
{
    boost::thread t([] {});
    t.join();

    return 0;
}

