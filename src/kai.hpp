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
const auto make_hb = [] { return std::make_unique<kiq::keepalive>(); };
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
    if (msg->type() > kiq::constants::IPC_KEEPALIVE_TYPE)
      klog().t("Message received: {}", msg->to_string());

    // ☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰Request Handlers☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰
    const auto handler = handler_t
    {
      { kiq::constants::IPC_KIQ_MESSAGE,    [this](auto&& ipc_msg) //--------------------------------
        {
          klog().d("KIQ msg ignored: {}", ipc_msg->to_string());
        }
      },
      { kiq::constants::IPC_KEEPALIVE_TYPE, [this](auto&&)         //--------------------------------
        {
          static unsigned int hb_count = 0;

          if (++hb_count % 400 == 0)
            klog().t("Heartbeats: {}", hb_count);
          endpoint_.send_ipc_message(make_hb());
        }
      },
      { kiq::constants::IPC_OK_TYPE,        [this](auto&&)         //--------------------------------
        {
          klog().d("Received OKAY from KIQ");
        }
      },
      { kiq::constants::IPC_PLATFORM_TYPE,  [this](auto&& ipc_msg) //--------------------------------
        {
          klog().d("Handling Platform Type Message");
          const auto p_msg = static_cast<kiq::platform_message*>(ipc_msg.get());
          generate(p_msg->content(), p_msg->id());
          messages_.insert_or_assign(p_msg->id(), std::move(ipc_msg));
        }
      },
      { kiq::constants::IPC_STATUS,         [this](auto&&)         //--------------------------------
        {
          klog().d("Received IPC_STATUS. Reconnecting");
          endpoint_.send_ipc_message(std::make_unique<kiq::okay_message>());
          endpoint_.reconnect();
        }
      },
    };
    // ☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰☰
    try
    {
      handler.at(msg->type())(std::move(msg));
    }
    catch (const std::exception& e)
    {
      klog().e("Caught exception while handling IPC: {}", e.what());
    }
  })
  {
    run();
  }

  //-----------------------------------------------------------------------------
  void add(std::string_view id, const session::exchange_t& exchange)
  {
    using info_t = kiq::platform_info;

    if (!has_session(g_id))
      sessions_.insert({g_id, session{}});
    sessions_.at(g_id).dialogue.push_back(exchange);

    auto&& out_msg = std::make_unique<info_t>("sentinel", exchange.second, "generated", id.data());
    klog().i("Sending {} this IPC: {}", endpoint_.get_addr(), out_msg->to_string());

    endpoint_.send_ipc_message(std::move(out_msg));
  }
  //-----------------------------------------------------------------------------
  void generate(const std::string& s, const std::string& id = "")
  {
    klog().d("generate request received");

    generating_ = true;

    fut_ = std::async(std::launch::async, [this, s, id]()
    {
      auto value = decode(post(s, url));
      if (value.empty())
      {
        klog().d("Failed to generate a response. Trying once more");
        value = decode(post(s, url));
      }

      add(id, session::exchange_t{s, value});

      generating_ = false;
    });

    klog().d("Future created");
  }

 private:
  //-----------------------------------------------------------------------------
  bool has_session(uint32_t id) const { return sessions_.find(id) != sessions_.end(); }
  void run()
  {
    while (true)
    {
      endpoint_.run();

      if (fut_.valid() && !generating_)
      {
        klog().d("Rejoining worker thread");
        fut_.get();
      }
    }
  }

  //--------------------------
  using fut_t = std::future<void>;

  sessions_t sessions_;
  endpoint   endpoint_;
  messages_t messages_;
  fut_t      fut_;
  bool       generating_{false};
};
