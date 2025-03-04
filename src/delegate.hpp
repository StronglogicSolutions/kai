#pragma once

#include <deque>
#include <variant>
#include <type_traits>
#include <functional>
#include <kproto/ipc.hpp>
#include "kutils.hpp"

//-------------------------------------------------------------------------------------------------
enum class task_t
{
  spike       = 0x00,
  development = 0x01,
  defect      = 0x02
};
//-------------------------------------------------------------------------------------------------
using task_variant_t = std::variant<
  std::monostate,
  std::integral_constant<task_t, task_t::spike>,
  std::integral_constant<task_t, task_t::development>,
  std::integral_constant<task_t, task_t::defect>>;
//-------------------------------------------------------------------------------------------------
using ipc_ptr_t  = kiq::task*;
using defct_constant = std::integral_constant<task_t, task_t::defect>;
using spike_constant = std::integral_constant<task_t, task_t::spike>;
using devel_constant = std::integral_constant<task_t, task_t::development>;
using defct_fn_t = std::function<void(defct_constant, ipc_ptr_t)>;
using spike_fn_t = std::function<void(spike_constant, ipc_ptr_t)>;
using devel_fn_t = std::function<void(devel_constant, ipc_ptr_t)>;
using genrt_fn_t = std::function<void(std::string_view)>;
using pending_queue_t = std::deque<std::string>;
using req_fn_t   = std::function<void(std::string)>;
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
//-------------------------------------------------------------------------------------------------
class task_handler
{
  public:
    task_handler(req_fn_t fn)
    : request_generation(fn)
    {
    }

    void operator()(defct_constant, ipc_ptr_t msg) const
    {
      auto tech = msg->tech();
      auto logs = msg->logs();
      auto desc = msg->description();

      auto blame_tech = [tech, desc]
      {
        std::string identified;
        auto technologies = kutils::split(tech, '<');
        for (const auto& tech : technologies)
          ;// TODO: ask AI if tech is likely the problem
        return identified;
      };

      auto solved_or_easy = [tech, desc, logs]
      {
      };
    }
    //-------------------------------
    void operator()(spike_constant, ipc_ptr_t msg) const
    {
    }
    //-------------------------------
    void operator()(devel_constant, ipc_ptr_t msg) const
    {
    }
    //-------------------------------
    void operator()(std::monostate, ipc_ptr_t msg) const
    {
    }
  private:
    req_fn_t request_generation;
};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class delegate
{
  public:

    delegate(genrt_fn_t fn)
    : gen_fn_(fn)
    {}
    //-------------------------------
    void process(kiq::ipc_message::u_ipc_msg_ptr u_ipc_msg, task_t type_of_task)
    {
      auto* msg = static_cast<ipc_ptr_t>(u_ipc_msg.get());
      std::visit(
        [this, msg](auto task_type) { handler_(task_type, msg); },
        to_variant(type_of_task));
    }
    //-------------------------------
    void on_generation(const std::string& generated)
    {
      if (queue_.empty())
        (void)("We have an error!");

      auto prompt = queue_.front();
      queue_.pop_front();
      // TODO: Handle the response
    }
  private:
    task_handler handler_{[this](std::string s) { queue_.push_back(s); gen_fn_(s); }};
    genrt_fn_t                 gen_fn_;
    pending_queue_t            queue_;

};

// for DEFECT:
// - types of technologies
// - description of issue
// - description of standard by which this issue would be solved
// - technologies
// - logs
//
