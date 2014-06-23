#ifndef PROGRAM_OPTIONS_H_
#define PROGRAM_OPTIONS_H_

#include <string>

class ProgramOptions
{
public:
  ProgramOptions(const int argc, const char* const argv[]);

  operator bool() const { return return_; }

  std::string hex_filename() const { return hex_filename_; }

private:
  ProgramOptions() {}
  bool return_;

  std::string hex_filename_;
};

#endif // PROGRAM_OPTIONS_H_