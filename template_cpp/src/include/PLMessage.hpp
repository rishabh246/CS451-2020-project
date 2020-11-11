#pragma once
#include <NetworkMessage.hpp>
#include <cerrno>
#include <clocale>
#include <stdlib.h>
#include <string>

#define PLHeaderLen 5 /* Uses 6 characters */

enum PLMessageType {
  TX,
  ACK
}; /* Types of message that the application layer can send */

class PLMessage {
public:
  PLMessageType type;
  NetworkMessage msg;

  PLMessage(PLMessageType type, NetworkMessage msg) : type(type), msg(msg) {}

  PLMessage(const PLMessage &msg) : type(msg.type) { this->msg = msg.msg; }

  std::string stringify() {
    std::string text;
    if (type == TX)
      text = "[TX ]" + msg.stringify();
    else
      text = "[ACK]" + msg.stringify();
    return text;
  }
};

PLMessage PLUnmarshall(char *buf, unsigned long len);

PLMessage PLUnmarshall(char *buf, unsigned long len) {
  if (len < PLHeaderLen) {
    throw std::runtime_error("Unmarshalling mangled PLmessage" +
                             std::string(std::strerror(errno)));
  }
  PLMessageType type;
  std::string str(buf);
  if (str.substr(0, PLHeaderLen) == "[TX ]")
    type = TX;
  else
    type = ACK;
  return PLMessage(type,
                   NetworkUnmarshall(buf + PLHeaderLen, len - PLHeaderLen));
}

PLMessage createAck(PLMessage msg);

PLMessage createAck(PLMessage msg) {
  PLMessage ack = msg;
  ack.type = ACK;
  /* Not ideal, but switch of sender, receiver is done by FLL */
  return ack;
}