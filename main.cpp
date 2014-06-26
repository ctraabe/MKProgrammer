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
  if (!hex)
    return 1;

  int tx_block_size = 4096;
  for (int bytes_sent = 0; bytes_sent < hex.size(); bytes_sent += tx_block_size)
  {
    uint8_t tx_block[tx_block_size + 2];  // includes 16-bit CRC
    CRC16 crc;
    for (int i = 0; i < tx_block_size; ++i)
    {
      tx_block[i] = hex.GetByte();
      crc.Update(tx_block[i]);
    }
    tx_block[tx_block_size] = crc.result() >> 8;
    tx_block[tx_block_size+1] = crc.result() & 0xFF;
  }

  return 0;
}
