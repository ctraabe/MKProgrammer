// This class employs Mikrokopter's bootloader communication protocol as
// described here: http://www.mikrokopter.de/ucwiki/en/BootLoader

#include "mk_comms.hpp"

#include <chrono>
#include <iostream>
#include <thread>

// ============================================================================+
// Public functions:

bool MKComms::RequestBLComms()
{
  bool responded = false;

  std::cout << "Sending device reset request." << std::endl;

  RequestDeviceReset();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  std::cout << "Waiting for MikroKopter bootloader." << std::flush;

  // Begin sending requests for bootloader comms.
  for (int seconds = 0; !responded && (seconds < 10); ++seconds)
  {
    constexpr int kPingFrequency = 10;  // Hz
    constexpr int kPingDuration = 1;  // Seconds
    for (int i = 0; !responded && (i < kPingFrequency * kPingDuration); ++i)
    {
      serial_.SendByte(0x1B);
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      serial_.SendByte(0xAA);
      std::this_thread::sleep_for(std::chrono::milliseconds(80));
      const uint8_t bootloader_init_string[4] = { 'M', 'K', 'B', 'L' };
      responded = CheckResponse(bootloader_init_string, 4);
    }
    std::cout << "." << std::flush;
  }
  std::cout << std::endl;

  if (!responded)
  {
    std::cerr << "ERROR: No response from the Mikrokopter device.\n";
    std::cerr << "Try removing power from the device then reapply power once"
      << " this program starts waiting for the bootloader." << std::endl;
    return false;
  }

  // Read the device signature.
  serial_.SendByte('t');
  uint8_t signature[2];
  if (!GetResponse(signature, 2, "Signature"))
    return false;

  // Process the device signature.
  // TODO: check that the device matches the hex file
  switch (signature[0])
  {
    case DEVICE_TYPE_MEGA644:
      std::cout << "FlightCtrl w/ ATMega644" << std::endl;
      break;
    case DEVICE_TYPE_MEGA1284:
      std::cout << "FlightCtrl w/ ATMega1284" << std::endl;
      break;
    case DEVICE_TYPE_STR911:
      std::cout << "NaviCtrl w/ STR911" << std::endl;
      break;
    default:
      std::cerr << "ERROR: Unsupported device." << std::endl;
      return false;
      break;
  }
  device_type_ = static_cast<DeviceType>(signature[0]);

  // Set the device.
  serial_.SendByte('T');
  serial_.SendByte(signature[0]);
  uint8_t okay[1];
  if (!GetResponse(okay, 1, "Set device"))
    return false;
  if (okay[0] != 0x0D)
  {
    std::cerr << "ERROR: Device did not accept request to set device type."
      << std::endl;
    return false;
  }

  // Read the bootloader version.
  serial_.SendByte('V');
  uint8_t version[2];
  if (!GetResponse(version, 2, "Bootloader version"))
    return false;
  std::cout << "MikroKopter bootloader V" << version[0] << "." << version[1]
    << std::endl;

  // Read the devices programming block size.
  serial_.SendByte('b');
  uint8_t program_block_size[3];
  if (!GetResponse(program_block_size, 3, "Program block size"))
    return false;
  if (program_block_size[0] != 'Y')
  {
    std::cerr << "ERROR: Unexpected response to request for program block size."
      << std::endl;
    return false;
  }
  program_block_size_ = (program_block_size[1] << 8) | program_block_size[2];
  std::cout << "Program block size: " << program_block_size_ << std::endl;

  return true;
}

bool MKComms::RequestClearFlash(const int blocks_to_clear) const
{
  if (device_type_ == DEVICE_TYPE_STR911)
  {
    const int bytes_to_clear = blocks_to_clear * program_block_size_;
    uint8_t header[4] = {
      'X',
      (uint8_t)((bytes_to_clear >> 16) & 0xFF),
      (uint8_t)((bytes_to_clear >> 8) & 0xFF),
      (uint8_t)(bytes_to_clear & 0xFF)
    };
    serial_.SendBuffer(header, sizeof(header));
    uint8_t okay[1];
    if (!GetResponse(okay, 1, "Set clear size"))
      return false;
    if (okay[0] != 0x0D)
    {
      std::cerr << "ERROR: Device did not accept request to set clear size."
        << std::endl;
      return false;
    }
    std::cout << "Requesting " << bytes_to_clear << " bytes to be cleared."
      << std::endl;
  }

  serial_.SendByte('e');
  uint8_t okay[1];
  if (!GetResponse(okay, 1, "Clear flash"))
    return false;
  if (okay[0] != 0x0D)
  {
    std::cerr << "ERROR: Device did not accept request to clear flash."
      << std::endl;
    return false;
  }
  std::cout << " done" << std::endl;
  return true;
}

bool MKComms::RequestAddress(const int address) const
{
  uint8_t header[3] = {
    'A',
    (uint8_t)((address >> 8) & 0xFF),
    (uint8_t)(address & 0xFF),
  };
  serial_.SendBuffer(header, sizeof(header));
  uint8_t okay[1];
  if (!GetResponse(okay, 1, "Set address"))
    return false;
  if (okay[0] != 0x0D)
  {
    std::cerr << "ERROR: Device did not accept request to set address."
      << std::endl;
    return false;
  }
  return true;
}

bool MKComms::SendProgramBlock(const uint8_t* const block, const int crc) const
{
  uint8_t header[4] = {
    'B',
    (uint8_t)((program_block_size_ >> 8) & 0xFF),
    (uint8_t)(program_block_size_ & 0xFF),
    'F'
  };
  serial_.SendBuffer(header, sizeof(header));

  serial_.SendBuffer(block, program_block_size_);

  uint8_t crc_buffer[2];
  crc_buffer[0] = crc >> 8;
  crc_buffer[1] = crc & 0xFF;
  serial_.SendBuffer(crc_buffer, sizeof(crc_buffer)) > 0;
  uint8_t okay[1];
  if (!GetResponse(okay, 1, "Block programming"))
    return false;
  if (okay[0] != 0x0D)
  {
    std::cerr << "ERROR: Device responded to CRC with " << (int)okay[0]
      << std::endl;
    return false;
  }
  return true;
}

bool MKComms::Exit() const
{
  serial_.SendByte('E');
}


// ============================================================================+
// Private  functions:

int MKComms::RequestDeviceReset() const
{
  // Message contains: sync char "#", address 0 + "a" = "a", reset command "R",
  // two-bit checksum, and 3 end-of-message chars "\r".
  uint8_t reset_request[8] = { '#', 'a', 'R', 0x40, 0x53, '\r', '\r', '\r' };
  return serial_.SendBuffer(reset_request, sizeof(reset_request));
}

bool MKComms::CheckResponse(const uint8_t* const expected_response,
  const int expected_response_length)
{
  constexpr int kBufferSize = 255;
  uint8_t rx_buffer[kBufferSize];
  int rx_bytes_read;

  rx_bytes_read = serial_.Read(rx_buffer, kBufferSize);
  for (int i = 0; i < rx_bytes_read; ++i)
  {
    if (rx_buffer[i] == expected_response[expected_response_index_])
    {
      if (++expected_response_index_ == expected_response_length)
      {
        expected_response_index_ = 0;
        return true;
      }
    }
    else
    {
      expected_response_index_ = 0;
    }
  }
  return false;
}

bool MKComms::GetResponse(uint8_t* const response, const int response_length,
  const std::string &request_string) const
{
  constexpr int kBufferSize = 255;
  uint8_t rx_buffer[kBufferSize];
  int rx_bytes_read, total_bytes_read = 0;

  // Poll the serial port for up to 1 second(ish).
  constexpr int kPollingDuration = 5;  // Seconds
  constexpr int kPollingFrequency = 100;  // Hz
  for (int i = 0; (total_bytes_read < response_length)
    && (i < (kPollingFrequency * kPollingDuration)); ++i)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000
      / kPollingFrequency));
    rx_bytes_read = serial_.Read(rx_buffer, kBufferSize);
    if ((total_bytes_read + rx_bytes_read) <= response_length)
    {
      for (int j = 0; j < rx_bytes_read; ++j)
        response[total_bytes_read+j] = rx_buffer[j];
    }
    total_bytes_read += rx_bytes_read;
  }

  if (total_bytes_read != response_length)
  {
    std::cerr << "ERROR: " << request_string << " request expected "
      << response_length << " byte(s) in response, got " << total_bytes_read
      << "." << std::endl;
    return false;
  }
  return true;
}

void MKComms::Close()
{
  serial_.Close();
}
