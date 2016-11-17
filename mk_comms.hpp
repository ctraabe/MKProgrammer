// This class employs Mikrokopter's bootloader communication protocol as
// described here: http://www.mikrokopter.de/ucwiki/en/BootLoader

#ifndef MK_COMMS_H_
#define MK_COMMS_H_

#include <string>

#include "serial.hpp"

class MKComms
{
public:
  enum DeviceType
  {
    DEVICE_TYPE_UNSUPPORTED = 0,
    DEVICE_TYPE_MEGA644 = 0x74,
    DEVICE_TYPE_MEGA1284 = 0x7A,
    DEVICE_TYPE_STR911 = 0xE0,
  };

  MKComms(const std::string &comport)
    : serial_(comport, 57600)
    , device_type_(DEVICE_TYPE_UNSUPPORTED)
    , expected_response_index_(0)
    , program_block_size_(0) {}

  operator bool() const { return serial_; }
  int program_block_size() const { return program_block_size_; }

  bool RequestBLComms(const std::string &hex_filename);
  bool RequestClearFlash(const int bytes_to_clear) const;
  bool SendProgram(const uint8_t* const program, const int size) const;
  bool Exit() const;
  void Close();

private:
  bool RequestAddress(const int address) const;
  bool SendProgramBlock(const uint8_t* const block) const;
  int RequestDeviceReset() const;
  bool CheckResponse(const uint8_t* const expected_response,
    const int expected_response_length);
  int GetResponse(uint8_t* const response, const int min_response_length,
    const int max_response_length, const std::string &request_string) const;

  Serial serial_;
  enum DeviceType device_type_;
  int program_block_size_;
  int expected_response_index_;
};

#endif // MK_COMMS_H_