#include "program_options.hpp"

#include <iostream>
#include <boost/program_options.hpp>

using namespace boost::program_options;

ProgramOptions::ProgramOptions(const int argc, const char* const argv[])
  : hex_filename_("input.hex")
  , return_(false)
{
  try
  {
    // Declare the supported options.
    options_description visible("Allowed options");
    visible.add_options()
      ("help,h", "produce help message")
      ;

    // Hidden options, will not be shown to the user.
    options_description hidden("Hidden options");
    hidden.add_options()
      ("input-file", value<std::string>(), "input file")
      ;

    options_description cmdline_options;
    cmdline_options.add(visible).add(hidden);

    positional_options_description positional_options;
    positional_options.add("input-file", -1);

    variables_map vm;
    store(command_line_parser(argc, argv).options(cmdline_options)
      .positional(positional_options).run(), vm);
    notify(vm);

    if (vm.count("help"))
    {
      std::cout << visible << "\n";
      return_ = true;
    }

    if (vm.count("input-file"))
    {
      hex_filename_ = vm["input-file"].as<std::string>();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << "\n\n";
    return_ = true;
  }
}
