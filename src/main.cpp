#include "helpers.hpp"
#include "kai.hpp"
#include <kutils.hpp>

//--------------------------------------------------------
using namespace kiq::log;

static const uint32_t g_id = 123;
//--------------------------------------------------------
//-----------MAIN-----------------------------------------
//--------------------------------------------------------
int main(int argc, char** argv)
{
  std::string input;

  klogger::init("kai", "debug");

  kai ai;

  ai.add(g_id);

  set_handler([&ai] (const auto& query, const auto& response)
  {
    ai.add(g_id, { query, response });
  });

  for (;;)
  {
    klog().i("\nPrompt: ");
    std::getline(std::cin, input);
    if (input.find("exit") == 0)
      break;

    post(input, url);
  }

  ai.print();

  return 0;
}
