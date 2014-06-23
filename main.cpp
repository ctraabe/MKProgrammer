#include <iostream>

#include "crc16.hpp"
#include "intel_hex.hpp"
#include "program_options.hpp"

int main (const int argc, const char* const argv[])
{
  ProgramOptions program_options(argc, argv);
  if (program_options)
    return 1;

  IntelHex hex(program_options.hex_filename());

  for (int k = 0; k < 3; k++) {
    int counter = 0;
    CRC16 crc;
    while(counter < 4096) {
      for (int i = 0; i < hex.GetDataCount(); ++i)
        crc.Update(hex.GetData(i));
      counter += hex.GetDataCount();
      hex.Next();
    }
    std::cout << std::hex << crc.result() << std::endl;
  }

  return 0;
}
