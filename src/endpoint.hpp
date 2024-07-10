#pragma once

#include <kproto/ipc.hpp>
#include <logger.hpp>

using namespace kiq::log;
static const char* RX_ADDR{"tcp://0.0.0.0:28485"};
static const char* TX_ADDR{"tcp://0.0.0.0:28474"};

using ipc_msg_cb_t = std::function<void(kiq::ipc_message::u_ipc_msg_ptr)>;

class endpoint : public kiq::IPCTransmitterInterface
{
 public:
  endpoint(ipc_msg_cb_t cb)
  : context_(1),
  on_recv_(cb)
  {
    connect();

    kiq::set_log_fn([](const auto& s) { klog().i("{}", s); });
  }
  //--------------------------------------------------------
  virtual ~endpoint() final
  {

  }
  //--------------------------------------------------------
  std::string
  get_addr() const
  {
    return tx_.get(zmq::sockopt::last_endpoint);
  }
  //--------------------------------------------------------
  void connect()
  {
    rx_ = zmq::socket_t(context_, ZMQ_ROUTER);
    tx_ = zmq::socket_t(context_, ZMQ_DEALER);

    rx_.set(zmq::sockopt::linger, 0);
    tx_.set(zmq::sockopt::linger, 0);
    rx_.set(zmq::sockopt::routing_id, "kai_daemon");
    tx_.set(zmq::sockopt::routing_id, "kai");
    rx_.set(zmq::sockopt::tcp_keepalive, 1);
    tx_.set(zmq::sockopt::tcp_keepalive, 1);
    rx_.set(zmq::sockopt::tcp_keepalive_idle,  300);
    tx_.set(zmq::sockopt::tcp_keepalive_idle,  300);
    rx_.set(zmq::sockopt::tcp_keepalive_intvl, 300);
    tx_.set(zmq::sockopt::tcp_keepalive_intvl, 300);

    rx_.bind   (RX_ADDR);
    tx_.connect(TX_ADDR);

    klog().d("Sending greeting to {}", get_addr());

    send_ipc_message(std::make_unique<kiq::status_check>());
  }
  //--------------------------------------------------------
  void recv(zmq::socket_t& socket)
  {
    using buffers_t = std::vector<kiq::ipc_message::byte_buffer>;

    zmq::message_t  identity;
    if (!socket.recv(identity) || identity.empty())
    {
      klog().e("Failed to receive IPC: No identity");
      return;
    }

    buffers_t      buffer;
    zmq::message_t msg;
    int            more_flag{1};

    while (more_flag && socket.recv(msg))
    {
      buffer.push_back({static_cast<char*>(msg.data()), static_cast<char*>(msg.data()) + msg.size()});
      more_flag = socket.get(zmq::sockopt::rcvmore);
    }

    on_recv_(kiq::DeserializeIPCMessage(std::move(buffer)));

  }
  //--------------------------------------------------------
  void
  run()
  {
    using namespace kiq;

    static const auto timeout   = std::chrono::milliseconds(300);

    while (true)
    {
      uint8_t           poll_mask = {0x00};
      zmq::pollitem_t   items[]   = { { tx_, 0, ZMQ_POLLIN, 0},
                                      { rx_, 1, ZMQ_POLLIN, 0} };
      zmq::poll(&items[0], 2, timeout);

      if (items[0].revents & ZMQ_POLLIN)
        recv(tx_);
      if (items[1].revents & ZMQ_POLLIN)
        recv(rx_);
    }
  }

 protected:
  virtual zmq::socket_t& socket() final { return tx_; }
  void on_done() final { (void)(0); }

 private:
  zmq::context_t    context_;
  zmq::socket_t     rx_;
  zmq::socket_t     tx_;
  ipc_msg_cb_t      on_recv_;
};