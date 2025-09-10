#pragma once

#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <type_traits>
#include <functional>
#include <kproto/ipc.hpp>
#include "helpers.hpp"
#include "task_utils.hpp"


//-------------------------------------------------------------------------------------------------
enum class task_t
{
  spike       = 0x00,
  development = 0x01,
  defect      = 0x02
};

using task_variant_t = std::variant<
  std::monostate,
  std::integral_constant<task_t, task_t::spike>,
  std::integral_constant<task_t, task_t::development>,
  std::integral_constant<task_t, task_t::defect>>;
//-------------------------------------------------------------------------------------------------
using ipc_ptr_t = kiq::task*;

using defct_constant = std::integral_constant<task_t, task_t::defect>;
using spike_constant = std::integral_constant<task_t, task_t::spike>;
using devel_constant = std::integral_constant<task_t, task_t::development>;
using defct_fn_t = std::function<void(defct_constant, ipc_ptr_t)>;
using spike_fn_t = std::function<void(spike_constant, ipc_ptr_t)>;
using devel_fn_t = std::function<void(devel_constant, ipc_ptr_t)>;
using genrt_fn_t = std::function<void(std::string_view)>;
using pending_queue_t = std::deque<std::string>;
using req_fn_t = std::function<void(std::string)>;
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static task_variant_t to_variant(task_t task_type)
{
    switch (task_type)
    {
        case task_t::spike:        return std::integral_constant<task_t, task_t::spike>{};
        case task_t::development:  return std::integral_constant<task_t, task_t::development>{};
        case task_t::defect:       return std::integral_constant<task_t, task_t::defect>{};
        default:                   return std::monostate{};
}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class task_handler
{
public:
task_handler(req_fn_t fn)
: request_generation(fn)
{}

void operator()(defct_constant, ipc_ptr_t msg) const
{
    const auto tech = msg->tech();
    const auto logs = msg->logs();
    const auto desc = msg->description();

    const auto& result = search_google(tech);

    if (result.empty())
      throw std::runtime_error("Failed to get search response");

    std::string prompt;
    for (const auto& value : result)
    {
      klog().d("Search result: {} : {}", value.title, value.snippet);
      if (is_english(value.title) && is_recent(find_recent_date(value.snippet)))
      {
          prompt = "We have a problem involving '" + tech + "'. The problem can be described as '" + desc + "'. The logs from an occurrence of this problem are '" + logs + "'. Please generate a solution.";
          klog().i("Generated prompt: {}", prompt);
      }
      else
        klog().e("The results are not in English or are not recent.");
    }

    if (prompt.empty())
      klog().e("No prompt generated");
    else
      request_generation(prompt);
}

void operator()(spike_constant, ipc_ptr_t msg) const // NOTE: SPIKE
{
    const auto tech = msg->tech();
    const auto logs = msg->logs();
    const auto desc = msg->description();

    auto final_report = [logs, desc]
    {
        std::string report;
        report += "Spike task:\n";
        report += "Description:\n";
        report += desc + "\n";
        report += "Logs:\n";
        report += logs + "\n";

        return report;
    }();

    request_generation(final_report);
}

void operator()(devel_constant, ipc_ptr_t msg) const // NOTE: DEVELOPMENT
{
    auto tech = msg->tech();
    auto logs = msg->logs();
    auto desc = msg->description();

    auto final_report = [tech, logs, desc]
    {
        std::string report;
        report += "Development task:\n";
        report += "Technology:\n";
        report += tech + "\n";
        report += "Description:\n";
        report += desc + "\n";
        report += "Logs:\n";
        report += logs + "\n";
        return report;
    }();

    request_generation(final_report);
}

void operator()(std::monostate, ipc_ptr_t msg) const // WARNING: UNKNOWN
{
    auto tech = msg->tech();
    auto logs = msg->logs();
    auto desc = msg->description();

    auto final_report = [tech, logs, desc]
    {
        std::string report;
        report += "Unknown task type:\n";
        report += "Technology:\n";
        report += tech + "\n";
        report += "Description:\n";
                 report += desc + "\n";
        report += "Logs:\n";
        report += logs + "\n";
        return report;
    }();

    request_generation(final_report);
}
private:
req_fn_t request_generation;
};
//-------------------------------------------------------------------------------------------------
class delegate
{
public:
delegate(genrt_fn_t fn)
: gen_fn_(fn)
{}
//---------------
void process(kiq::ipc_message::u_ipc_msg_ptr u_ipc_msg, task_t type_of_task)
{
    auto* msg = static_cast<ipc_ptr_t>(u_ipc_msg.get());
    std::visit(
        [this, msg](auto task_type) { handler_(task_type, msg); },
        to_variant(type_of_task));
}
//---------------
bool do_test()
{
  task_t type = task_t::defect;
  kiq::task req{"my ID","this is my description", "Defect", "CEF", "log1,log2"};
  process(std::make_unique<kiq::ipc_message>(req), type);

  return true;
}
//---------------
void on_generation(const std::string& generated)
{
  if (queue_.empty())
    throw std::runtime_error(
      std::string{"Genertion received, but queue is empty! Generated: {}" + generated});

  auto prompt = queue_.front();
  queue_.pop_front();
  klog().t("\nOriginal prompt: {}\nGenerated response: {}", prompt, generated);
    // TODO: Handle the response
}
//---------------
//---------------
private:
  task_handler    handler_{[this](auto s) { queue_.push_back(s); gen_fn_(s); }};
  genrt_fn_t      gen_fn_;
  pending_queue_t queue_;
};
