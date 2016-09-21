/*
local_settings:
    build:
        cxx_flags: -std=c++11
dependencies:
    pvt.cppan.demo.ninetainedo.sery
*/

#include <Sery/Buffer.hh>
#include <Sery/Stream.hh>
#include <fstream>

int             main()
{
  Sery::Buffer  buffer;
  Sery::Stream  stream(buffer, Sery::LittleEndian);
  std::ofstream file("BinaryFile", std::ofstream::binary);
  Sery::int32   magic = 0x11223344;
  std::string   str("Hello world!");

  stream << magic << str;
  file.write(buffer.data(), buffer.size());
  file.close();
}
