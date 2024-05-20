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
struct session
{
  using exchange_t = std::pair<std::string, std::string>;
  using dialogue_t = std::vector<exchange_t>;

  dialogue_t dialogue;
};

using sessions_t = std::map<uint32_t, session>;

class kai
{
 public:

  kai()
  : endpoint_([this](kiq::ipc_message::u_ipc_msg_ptr msg)
  {
    using handler_t = std::map<uint8_t, std::function<void(kiq::ipc_message::u_ipc_msg_ptr)>>;
    klog().t("Message received:\n{}", msg->to_string());

    const auto handler = handler_t
    {
      { kiq::constants::IPC_KIQ_MESSAGE,  [this](const auto ipc_msg)
        {
          generate(get_payload(static_cast<kiq::kiq_message*>(ipc_msg.get())->payload()).query());
        }
      },
      { kiq::constants::IPC_STATUS,       [this](const auto ipc_msg)
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
  void add(uint32_t id)
  {
    if (has_session(id))
      klog().w("Session {} already exists. Ignored.", id);
    else
      sessions_.insert({id, session{}});
  }
  //-----------------------------------------------------------------------------
  void add(uint32_t id, const session::exchange_t& exchange)
  {
    if (!has_session(id))
      add(id);
    sessions_.at(id).dialogue.push_back(exchange);

    endpoint_.send_ipc_message(std::make_unique<kiq::platform_info>(
      "kai",
      std::string{exchange.first + "\n\n" + exchange.second},
      "generated"));
  }
  //-----------------------------------------------------------------------------
  session::exchange_t
  find(uint32_t id, const std::string& query) const
  {
    if (!has_session(id))
      klog().w("Can't find dialogue: session {} doesn't exist", id);
    else
      for (const auto& exchange : sessions_.at(id).dialogue)
        if (exchange.first.find(query) != std::string::npos)
          return exchange;

    return session::exchange_t{};
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
  void generate(std::string_view s)
  {
    post(s, url);
  }

 private:
  //-----------------------------------------------------------------------------
  bool has_session(uint32_t id) const { return sessions_.find(id) != sessions_.end(); }

  //--------------------------
  sessions_t sessions_;
  endpoint   endpoint_;
};