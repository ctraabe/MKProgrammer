#include "intel_hex.hpp"

#include <iostream>

IntelHex::IntelHex(const std::string &hex_filename)
  : hex_filename_(hex_filename)
  , total_bytes_(0)
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

int IntelHex::GetByte()
{
  while (!end_of_file_)
  {
    if (current_line_bytes_read_ < current_line_byte_count_)
    {
      return GetData(current_line_bytes_read_++);
    }
    else
    {
      do {
        GetLine();
      } while (current_line_record_type_ == RECORD_TYPE_EXTENDED_ADDRESS);
    }
  }

  return -1;
}


// ============================================================================+
// Private  functions:

// Read the next line of the hex file.
bool IntelHex::GetLine()
{
  if (!hex_file_.is_open()) return false;
  current_line_position_ = hex_file_.tellg();
  if(end_of_file_ || std::getline(hex_file_, current_line_).eof()
    || (current_line_.length() < 12))
  {
    current_line_record_type_ = RECORD_TYPE_END_OF_FILE;
    current_line_byte_count_ = 0;
    current_line_bytes_read_ = 0;
    end_of_file_ = true;
  }
  else
  {
    // Read the record type and number of data bytes for the current line of the
    // hex file.
    constexpr int kRecordTypePos = 7;
    constexpr int kRecordTypeLen = 2;
    constexpr int kByteCountPos = 1;
    constexpr int kByteCountLen = 2;

    int raw_type = std::stoi(current_line_.substr(kRecordTypePos,
      kRecordTypeLen), nullptr, 16);
    if (raw_type >= 0 && raw_type < 4)
      current_line_record_type_ = static_cast<RecordType>(raw_type);
    else
      current_line_record_type_ = RECORD_TYPE_UNSUPPORTED;
    current_line_byte_count_ =  std::stoi(current_line_.substr(kByteCountPos,
      kByteCountLen), nullptr, 16);

    current_line_bytes_read_ = 0;

    end_of_file_ = current_line_record_type_ == RECORD_TYPE_END_OF_FILE;
  }

  return !end_of_file_;
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
// Read a data byte from the current line of the hex file (zero-indexed).
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

// Scan the hex file to discover any extended address blocks and do a little bit
// of error checking.
void IntelHex::Scan()
{
  if (!hex_file_.is_open()) return;

  // Make sure to start at the beginning of the file.
  GoToFirstLine();

  int line_number = 0;
  do
  {
    ++line_number;
    switch (current_line_record_type_)
    {
      case RECORD_TYPE_DATA:
      {
        int address = GetAddress()
          + extended_address_block_vector_.back().address_offset;
        if (address != total_bytes_)
        {
          std::cerr << "ERROR: Data at " << hex_filename_ << ": " << line_number
            << " is not contiguous with preceding data." << std::endl;
          Close();
          return;
        }
        total_bytes_ += current_line_byte_count_;
        break;
      }
      case RECORD_TYPE_EXTENDED_ADDRESS:
      {
        ExtendedAddressBlock extended_address_block;
        extended_address_block.hex_file_position = current_line_position_;
        extended_address_block.address_offset = (GetData(0) << 12)
          + (GetData(1) << 4);
        if (extended_address_block.address_offset != total_bytes_)
        {
          std::cerr << "ERROR: The extended address block 0x" << std::hex
            << extended_address_block.address_offset << " at " << hex_filename_
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
        std::cerr << "ERROR: Can't interpret text at " << hex_filename_ << ": "
          << line_number << "." << std::endl;
        Close();
        return;
        break;
      }
    }
  } while (GetLine());

  std::cout << hex_filename_ << " contains " << total_bytes_ << " bytes in "
    << extended_address_block_vector_.size() << " block(s)." << std::endl;
  GoToFirstLine();
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
  GetLine();
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
