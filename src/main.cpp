#include <cstring>
#include <filesystem>
#include <iostream>

#include "wall_clock.h"

int main(int argc, char *argv[])
{
  if (argc > 1)
  {
    if (argc == 2 && std::strcmp("--version", argv[1]) == 0)
    {
      std::cout << "Clock: " << PROJECT_VERSION << std::endl;
      return 0;
    }
    else
    {
      std::cerr << "Unknown option" << std::endl;
      return -1;
    }
  }
  else
  {
    try
    {
      wall_clock w_c{
          (std::filesystem::path{argv[0]}.parent_path() /
           HELP_RELATIVE_PATH)
              .string()};
      w_c.run();
    }
    catch (const char *error)
    {
      std::cout << "Exception: " << error << std::endl;
      return -1;
    }
    return 0;
  }
}
