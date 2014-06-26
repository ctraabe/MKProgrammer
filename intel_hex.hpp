#ifndef INTEL_HEX_H_
#define INTEL_HEX_H_

#include <fstream>
#include <string>
#include <vector>

class IntelHex
{
public:
  enum RecordType
  {
    RECORD_TYPE_DATA = 0,
    RECORD_TYPE_END_OF_FILE = 1,
    RECORD_TYPE_EXTENDED_ADDRESS = 2,
    RECORD_TYPE_UNSUPPORTED = 3,
  };

  IntelHex(const std::string &hex_file_name);

  operator bool() const { return hex_file_.is_open(); }

  // The number of program bytes recorded in the hex file
  int size() const { return total_bytes_; }

  // Read the next byte of the hex file (returns true if successful)
  int GetByte();

private:
  struct ExtendedAddressBlock {
    std::streampos hex_file_position;
    int address_offset;
  };

  // Get the next line of the hex file.
  bool GetLine();

  // Read various records from the current line of the hex file.
  int GetAddress() const;
  int GetData(int n) const;
  int GetChecksum() const;

  // Scan the hex file to discover any extended address blocks and do a little
  // bit of error checking.
  void Scan();

  void GoToFirstLine();
  void GoToFinalLine();
  void GoToPreviousLine();
  void GoToBeginningOfLine();
  void GoToLineAtPosition(std::streampos position);
  void Close();

  std::ifstream hex_file_;
  std::string hex_filename_;
  std::string current_line_;
  std::streampos current_line_position_;
  enum RecordType current_line_record_type_;
  int current_line_byte_count_;
  int current_line_bytes_read_;
  int total_bytes_;

  bool end_of_file_;
  std::vector<ExtendedAddressBlock> extended_address_block_vector_;
};

#endif // INTEL_HEX_H_