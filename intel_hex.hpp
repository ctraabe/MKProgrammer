#ifndef INTEL_HEX_H_
#define INTEL_HEX_H_

#include <fstream>
#include <string>

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

  // Is the 
  operator bool() const { return hex_file_.is_open(); }

  // Read the next line of the hex file (returns true if successful)
  bool Next();

  // Get the total number of data bytes recorded in this hex file.
  int TotalDataBytes();

  // Read various records from the current line of the hex file.
  int GetAddress() const;
  int GetDataCount() const;
  int GetData(int n) const;
  int GetChecksum() const;

private:
  struct ExtendedAddressBlock {
    std::streampos hex_file_position;
    int address_offset;
  };
  enum RecordType GetRecordType() const;

  void GoToFinalLine();
  void GoToPreviousLine();
  void GoToBeginningOfLine();
  void GoToLineAtPosition(std::streampos position);

  std::ifstream hex_file_;
  std::string current_line_;
  std::streampos current_line_position_;

  bool end_of_file_;
  int total_data_bytes_;
  ExtendedAddressBlock extended_address_block_[16];
};

#endif // INTEL_HEX_H_