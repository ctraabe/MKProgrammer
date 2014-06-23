#include <iostream>

#include "intel_hex.hpp"

int main (const int argc, const char* const argv[])
{
  IntelHex hex("sample.hex");

  std::cout << hex.TotalDataBytes() << std::endl;
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

  return 0;
}
