/*
local_settings:
    build:
        generator: Visual Studio 14 2015 Win64
        cxx_compiler_flags_debug: /MTd
        cxx_compiler_flags_release: /MT
dependencies:
  pvt.cppan.demo.boost.system: 1
*/

#include <boost/system/error_code.hpp>

int main()
{
    boost::system::error_code ec;
    return 0;
}
