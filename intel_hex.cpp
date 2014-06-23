#include "intel_hex.hpp"

IntelHex::IntelHex(const std::string &hex_file_name)
  : end_of_file_(false)
{
  hex_file_.open(hex_file_name);
  Next();
}

bool IntelHex::Next()
{
  current_line_position_ = hex_file_.tellg();
  end_of_file_ = (end_of_file_
    || std::getline(hex_file_, current_line_).eof()
    || (current_line_.length() < 12)
    || (GetRecordType() == RECORD_TYPE_END_OF_FILE));
  return end_of_file_;
}

int IntelHex::TotalDataBytes()
{
  std::streampos original_position = current_line_position_;
  GoToFinalLine();
  while (GetRecordType() != IntelHex::RECORD_TYPE_DATA)
  {
    GoToPreviousLine();
  }
  int return_value = GetAddress() + GetDataCount();
  GoToLineAtPosition(original_position);
  return return_value;
}

enum IntelHex::RecordType IntelHex::GetRecordType() const
{
  constexpr int kRecordTypePos = 7;
  constexpr int kRecordTypeLen = 2;

  int raw_value = std::stoi(current_line_.substr(kRecordTypePos,
    kRecordTypeLen), nullptr, 16);
  if (raw_value >= 0 && raw_value < 4)
    return static_cast<IntelHex::RecordType>(raw_value);
  else
    return RECORD_TYPE_UNSUPPORTED;
}

int IntelHex::GetAddress() const
{
  constexpr int kAddressPos = 3;
  constexpr int kAddressLen = 4;

  return std::stoi(current_line_.substr(kAddressPos, kAddressLen), nullptr, 16);
}

int IntelHex::GetDataCount() const
{
  constexpr int kByteCountPos = 1;
  constexpr int kByteCountLen = 2;

  return std::stoi(current_line_.substr(kByteCountPos, kByteCountLen), nullptr,
    16);
}

int IntelHex::GetData(int n) const
{
  constexpr int kDataPos = 9;

  return std::stoi(current_line_.substr(kDataPos + 2 * n, 2), nullptr, 16);
}

int IntelHex::GetChecksum() const
{
  // Note that length includes a carriage return at the end of the line.
  return std::stoi(current_line_.substr(current_line_.length() - 3, 2), nullptr,
    16);
}

void IntelHex::GoToFinalLine()
{
  hex_file_.seekg(-1, hex_file_.end);
  GoToBeginningOfLine();
}

void IntelHex::GoToPreviousLine()
{
  hex_file_.seekg(current_line_position_ - 1L);
  GoToBeginningOfLine();
}

void IntelHex::GoToBeginningOfLine()
{
  while (hex_file_.tellg() && hex_file_.peek() != ':')
    hex_file_.unget();
  end_of_file_ = false;
  Next();
}

void IntelHex::GoToLineAtPosition(std::streampos position)
{
  hex_file_.seekg(position);
  GoToBeginningOfLine();
}
