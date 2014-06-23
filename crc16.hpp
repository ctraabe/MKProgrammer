#ifndef CRC16_H_
#define CRC16_H_

#include <cinttypes>

class CRC16
{
public:
  CRC16() : crc_(0xFFFF) {};
  void Update(const uint8_t input);

  int result() { return crc_; }

private:
  uint16_t crc_;

};

#endif // CRC16_H_
