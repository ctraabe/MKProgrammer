#include "intel_hex.hpp"

#include <iostream>

IntelHex::IntelHex(const std::string &hex_filename)
  : hex_filename_(hex_filename)
  , end_of_file_(false)
{
  // Initialize the extended address block vector with a 0,0 entry.
  ExtendedAddressBlock extended_address_block = {};  // zeros
  extended_address_block_vector_.push_back(extended_address_block);

  // Open the hex file and scan it to discover any extended address blocks.
  hex_file_.open(hex_filename);
  if (hex_file_)
    Scan();
  else
    std::cerr << "ERROR: Couldn't open " << hex_filename << std::endl;
}

// ============================================================================+
// Public functions:

// Read the next line of the hex file.
bool IntelHex::Next()
{
  if (!hex_file_.is_open()) return false;
  current_line_position_ = hex_file_.tellg();
  end_of_file_ = (end_of_file_
    || std::getline(hex_file_, current_line_).eof()
    || (current_line_.length() < 12)
    || (GetRecordType() == RECORD_TYPE_END_OF_FILE));
  return !end_of_file_;
}

// -----------------------------------------------------------------------------
// Read the record type for the current line of the hex file.
enum IntelHex::RecordType IntelHex::GetRecordType() const
{
  if (!hex_file_.is_open()) return RECORD_TYPE_UNSUPPORTED;

  constexpr int kRecordTypePos = 7;
  constexpr int kRecordTypeLen = 2;

  int raw_value = std::stoi(current_line_.substr(kRecordTypePos,
    kRecordTypeLen), nullptr, 16);
  if (raw_value >= 0 && raw_value < 4)
    return static_cast<IntelHex::RecordType>(raw_value);
  else
    return RECORD_TYPE_UNSUPPORTED;
}

// -----------------------------------------------------------------------------
// Read the address for the current line of the hex file.
int IntelHex::GetAddress() const
{
  if (!hex_file_.is_open()) return -1;

  constexpr int kAddressPos = 3;
  constexpr int kAddressLen = 4;

  return std::stoi(current_line_.substr(kAddressPos, kAddressLen), nullptr, 16);
}

// -----------------------------------------------------------------------------
// Read the number of data bytes for the current line of the hex file.
int IntelHex::GetDataCount() const
{
  if (!hex_file_.is_open()) return -1;

  constexpr int kByteCountPos = 1;
  constexpr int kByteCountLen = 2;

  return std::stoi(current_line_.substr(kByteCountPos, kByteCountLen), nullptr,
    16);
}

// -----------------------------------------------------------------------------
// Read a data byte from the current line of the hex file.
// TODO: check that the requested index is valid
int IntelHex::GetData(int n) const
{
  if (!hex_file_.is_open()) return -1;

  constexpr int kDataPos = 9;

  return std::stoi(current_line_.substr(kDataPos + 2 * n, 2), nullptr, 16);
}

// -----------------------------------------------------------------------------
// Read the checksum for the current line of the hex file.
int IntelHex::GetChecksum() const
{
  if (!hex_file_.is_open()) return -1;

  // Note that length includes a carriage return at the end of the line.
  return std::stoi(current_line_.substr(current_line_.length() - 3, 2), nullptr,
    16);
}


// ============================================================================+
// Private  functions:

// Scan the hex file to discover any extended address blocks and do a little bit
// of error checking.
void IntelHex::Scan()
{
  if (!hex_file_.is_open()) return;

  // Make sure to start at the beginning of the file.
  GoToFirstLine();

  int last_address = 0, line_number = 0, total_data_bytes = 0;
  do
  {
    ++line_number;
    switch (GetRecordType())
    {
      case RECORD_TYPE_DATA:
      {
        int data_count = GetDataCount();
        total_data_bytes += data_count;
        last_address = GetAddress() + data_count;
        break;
      }
      case RECORD_TYPE_EXTENDED_ADDRESS:
      {
        ExtendedAddressBlock extended_address_block;
        extended_address_block.hex_file_position = current_line_position_;
        extended_address_block.address_offset = (GetData(0) << 12)
          + (GetData(1) << 4);
        if (extended_address_block.address_offset != last_address
          + extended_address_block_vector_.back().address_offset)
        {
          std::cerr << "ERROR: The extended address block 0x" << std::hex
            << extended_address_block.address_offset << " (" << hex_filename_
            << ": " << std::dec << line_number
            << ") is not contiguous with the preceding data." << std::endl;
          Close();
          return;
        }
        extended_address_block_vector_.push_back(extended_address_block);
        break;
      }
      default:
      {
        std::cerr << "ERROR: Can't interpret " << hex_filename_ << " line "
          << line_number << "." << std::endl;
        Close();
        return;
        break;
      }
    }
  } while (Next());

  if (total_data_bytes != last_address
    + extended_address_block_vector_.back().address_offset)
  {
    std::cout << total_data_bytes << " : " << last_address
    + extended_address_block_vector_.back().address_offset << std::endl;
    std::cerr << "ERROR: The number of bytes specified in " << hex_filename_
      << " does not match the address space." << std::endl;
    Close();
  }
  else
  {
    std::cout << hex_filename_ << " contains " << total_data_bytes
      << " bytes in "  << extended_address_block_vector_.size() << " block(s)."
      << std::endl;
    GoToFirstLine();
  }
}

// -----------------------------------------------------------------------------
// Go to the final line of the hex file (should an end of file record).
void IntelHex::GoToFirstLine()
{
  hex_file_.seekg(0, hex_file_.beg);
  GoToBeginningOfLine();
}

// -----------------------------------------------------------------------------
// Go to the final line of the hex file (should an end of file record).
void IntelHex::GoToFinalLine()
{
  hex_file_.seekg(-1, hex_file_.end);
  GoToBeginningOfLine();
}

// -----------------------------------------------------------------------------
// Go to the previous line of the hex file.
void IntelHex::GoToPreviousLine()
{
  if (current_line_position_ > 0)
    hex_file_.seekg(current_line_position_ - 1L);
  GoToBeginningOfLine();
}

// -----------------------------------------------------------------------------
// Go to the beginning of the current line (assuming it starts with a ':').
void IntelHex::GoToBeginningOfLine()
{
  while (hex_file_.tellg() && hex_file_.peek() != ':')
    hex_file_.unget();
  end_of_file_ = false;
  Next();
}

// -----------------------------------------------------------------------------
// Jump to the line at the specified stream position.
void IntelHex::GoToLineAtPosition(std::streampos position)
{
  hex_file_.seekg(position);
  GoToBeginningOfLine();
}

// -----------------------------------------------------------------------------
// Close the hex file and clear this object.
void IntelHex::Close()
{
  hex_file_.close();
  end_of_file_ = true;
}
