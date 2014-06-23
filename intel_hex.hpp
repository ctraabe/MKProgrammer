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

  // Is the 
  operator bool() const { return hex_file_.is_open(); }

  // Read the next line of the hex file (returns true if successful)
  bool Next();

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

  bool end_of_file_;
  std::vector<ExtendedAddressBlock> extended_address_block_vector_;
};

#endif // INTEL_HEX_H_