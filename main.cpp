#include <iostream>

#include "crc16.hpp"
#include "intel_hex.hpp"
#include "mk_comms.hpp"
#include "program_options.hpp"

int main (const int argc, const char* const argv[])
{
  // Parse command line options.
  ProgramOptions program_options(argc, argv);
  if (!program_options)
    return 1;

  // Open the hex file.
  IntelHex hex(program_options.hex_filename());
  if (!hex)
    return 1;

  // Open serial communications with a MikroKopter device (bootloader).
  MKComms mk_comms("/dev/ttyUSB0");
  if (!mk_comms)
    return 1;

  if (!mk_comms.RequestBLComms())
    return 1;

  // Calculate the number of programming blocks to be transmitted.
  const int block_size = mk_comms.program_block_size();
  const int block_count = (hex.size() - 1) / block_size + 1;

  // Clear the flash memory.
  if (!mk_comms.RequestClearFlash(block_count))
    return 1;
  if (!mk_comms.RequestAddress(0x0000))
    return 1;

  // Start sending the contents of the hex file to the MikroKopter device.
  for (int block_index = 0; block_index < block_count; ++block_index)
  {
    std::cout << "Programming block " << block_index + 1 << " of "
      << block_count << std::endl;

    uint8_t tx_block[block_size];
    CRC16 crc;
    for (int i = 0; i < block_size; ++i)
    {
      tx_block[i] = hex.GetByte();
      crc.Update(tx_block[i]);
    }
    mk_comms.SendProgramBlock(tx_block, crc.result());
  }

  mk_comms.Exit();

  return 0;
}
