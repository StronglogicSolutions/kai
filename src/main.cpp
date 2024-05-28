#include "helpers.hpp"
#include "kai.hpp"

//--------------------------------------------------------
using namespace kiq::log;

bool
is_cmd(std::string_view s, std::string_view cmd)
{
  return (s.find(cmd) == 0);
}
//--------------------------------------------------------
//-----------MAIN-----------------------------------------
//--------------------------------------------------------
int main(int argc, char** argv)
{
  std::string input;

  klogger::init("kai", "trace");

  kai ai;

  for (;;)
  {
    klog().i("\n\nPrompt:");
    std::getline(std::cin, input);
    if (is_cmd(input, "exit"))
      break;
    if (is_cmd(input, "print"))
      ai.print();

    ai.generate(input);
  }

  return 0;
}
