#include <iostream>

#include "intel_hex.hpp"
#include "program_options.hpp"

int main (const int argc, const char* const argv[])
{
  ProgramOptions program_options(argc, argv);
  if (program_options)
    return 1;

  IntelHex hex(program_options.hex_filename());
/*
  std::cout << hex.GetDataCount() << std::endl;
  for (int i = 0; i < hex.GetDataCount(); ++i)
  {
    std::cout << hex.GetData(i) << ' ';
  }
  std::cout << hex.GetChecksum() << std::endl;
  while (hex.Next())
  {
    std::cout << hex.GetAddress() << std::endl;
  }
  hex.Next();
  hex.Next();
  hex.Next();
  hex.Next();
*/
  return 0;
}
