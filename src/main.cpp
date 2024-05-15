#include "helpers.hpp"
#include "kai.hpp"
#include <kutils.hpp>

//--------------------------------------------------------
using namespace kiq::log;

static const uint32_t g_id = 123;

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

  set_handler([&ai] (const auto& query, const auto& response) { ai.add(g_id, { query, response }); });

  for (;;)
  {
    klog().i("\n\nPrompt:");
    std::getline(std::cin, input);
    if (is_cmd(input, "exit"))
      break;
    if (is_cmd(input, "print"))
      ai.print();

    post(input, url);
  }

  return 0;
}
