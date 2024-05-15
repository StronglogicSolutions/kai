#include <map>
#include <string>
#include <vector>
#include <exception>
#include <logger.hpp>

using namespace kiq::log;

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
 private:
  //-----------------------------------------------------------------------------
  bool has_session(uint32_t id) const { return sessions_.find(id) != sessions_.end(); }

  //--------------------------
  sessions_t sessions_;

};