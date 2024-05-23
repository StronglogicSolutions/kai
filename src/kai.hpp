#pragma once

#include <map>
#include <string>
#include <vector>
#include <exception>
#include "endpoint.hpp"

using namespace kiq::log;
//--------------------------------------------------------
static const char* url = "http://127.0.0.1:8080/completion"; // LLAMA AI SERVER
//--------------------------------------------------------
static const uint32_t g_id = 123;
//--------------------------------------------------------
struct session
{
  using exchange_t = std::pair<std::string, std::string>;
  using dialogue_t = std::vector<exchange_t>;

  dialogue_t dialogue;
};

using sessions_t = std::map<uint32_t, session>;
using messages_t = std::map<std::string, kiq::ipc_message::u_ipc_msg_ptr>;
class kai
{
 public:

  kai()
  : endpoint_([this](kiq::ipc_message::u_ipc_msg_ptr msg)
  {
    using handler_t = std::map<uint8_t, std::function<void(kiq::ipc_message::u_ipc_msg_ptr)>>;

    static const auto last_msg_idx = std::to_string(std::numeric_limits<uint32_t>::max());

    klog().t("Message received:\n{}", msg->to_string());

    const auto handler = handler_t
    {
      { kiq::constants::IPC_KIQ_MESSAGE,  [this](auto&& ipc_msg)
        {
          const auto payload = get_payload(static_cast<kiq::kiq_message*>(ipc_msg.get())->payload());

          if (payload.type() == "generate")
          {
            generate(payload.data(), last_msg_idx);
            messages_.insert_or_assign(last_msg_idx, std::move(ipc_msg));
          }
          else
            klog().w("Received unknown request name: {}", payload.type());
        }
      },
      { kiq::constants::IPC_PLATFORM_TYPE,  [this](auto&& ipc_msg)
        {
          const auto p_msg   = static_cast<kiq::platform_message*>(ipc_msg.get());
          const auto payload = get_payload(p_msg->args());

          if (payload.type() == "generate")
          {
            generate(payload.data());
            messages_.insert_or_assign(p_msg->id(), std::move(ipc_msg));
          }
          else
            klog().w("Received unknown request name: {}", payload.type());
        }
      },
      { kiq::constants::IPC_STATUS,       [this](auto&& ipc_msg)
        {
          endpoint_.send_ipc_message(std::make_unique<kiq::okay_message>());
          endpoint_.connect();
        }
      },
    };

    try
    {
      handler.at(msg->type())(std::move(msg));
    }
    catch (const std::exception& e)
    {
      klog().e("Caught exception while handling IPC: {}", e.what());
    }
  })
  {}

  //-----------------------------------------------------------------------------
  void add(std::string_view id, const session::exchange_t& exchange)
  {
    if (!has_session(g_id))
      sessions_.insert({g_id, session{}});
    sessions_.at(g_id).dialogue.push_back(exchange);

    const auto ipc_msg = messages_.at(id.data()).get();

    endpoint_.send_ipc_message(std::make_unique<kiq::platform_info>(
      "kai",
      std::string{exchange.first + "\n\n" + exchange.second},
      "generated"));
  }
  //-----------------------------------------------------------------------------
  void
  print() const
  {
    for (const auto& [id, session] : sessions_)
      for (const auto& exchange : session.dialogue)
        klog().d(
          "\n─────────────────────────────────────────────────────────────────────────────\
           \nSession {}\n\nQUERY: {}\n☰☰☰☰☰☰\nRESPONSE:\n\n{}\
           \n─────────────────────────────────────────────────────────────────────────────",
          id, exchange.first, exchange.second);
  }
  //-----------------------------------------------------------------------------
  void generate(std::string_view s, std::string_view id = "")
  {
    const auto response = post(s, url);
    const auto value    = decode(response);
    add(id, session::exchange_t{s, value});
  }

 private:
  //-----------------------------------------------------------------------------
  bool has_session(uint32_t id) const { return sessions_.find(id) != sessions_.end(); }

  //--------------------------
  sessions_t sessions_;
  endpoint   endpoint_;
  messages_t messages_;
};
