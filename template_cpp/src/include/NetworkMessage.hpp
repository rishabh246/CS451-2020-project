#pragma once
#include "AppMessage.hpp"

class NetworkMessage {
public:
  AppMessage app_msg;
  unsigned long sender;
  unsigned long receiver;

  NetworkMessage() : sender(0), receiver(0) {}

  NetworkMessage(AppMessage msg, unsigned long sender, unsigned long receiver)
      : app_msg(msg), sender(sender), receiver(receiver) {}

  NetworkMessage(const NetworkMessage &msg)
      : app_msg(msg.app_msg), sender(msg.sender), receiver(msg.receiver) {}

  bool operator==(const NetworkMessage &other) {
    return (this->app_msg == other.app_msg && this->receiver == other.receiver);
  }

  std::string stringify() {
    return stringify_host_ids(sender) + app_msg.stringify();
  }
};

NetworkMessage NetworkUnmarshall(char *buf, unsigned long len);
NetworkMessage NetworkUnmarshall(char *buf, unsigned long len) {
  if (len < IDStrLen)
  /* TODO: Need to guard against more complex errors */ {
    LOG(len);
    throw std::runtime_error("Unmarshalling mangled NetworkMessage" +
                             std::string(std::strerror(errno)));
  }
  unsigned long sender;
  std::string str(buf);
  sender = unmarshall_host_ids(str.substr(1, IDStrLen - 2));
  AppMessage msg = AppUnmarshall(buf + IDStrLen, len - IDStrLen);
  return NetworkMessage(msg, sender,
                        0); /* Not ideal, but this is set by the FLL */
}
