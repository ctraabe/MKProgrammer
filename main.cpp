#include <iostream>

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
  MKComms mk_comms(program_options.serial_port());
  if (!mk_comms)
    return 1;

  if (!mk_comms.RequestBLComms(program_options.hex_filename()))
    return 1;

  // Clear the flash memory.
  if (!mk_comms.RequestClearFlash(hex.size()))
    return 1;

  // Send the contents of the hex file to the device.
  if (!mk_comms.SendProgram(hex.program(), hex.size()))
    return 1;

  mk_comms.Exit();

  return 0;
}
