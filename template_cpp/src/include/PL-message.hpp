#pragma once
#include <FLLMessage.hpp>
#include <cerrno>
#include <clocale>
#include <stdlib.h>
#include <string>

enum PLMessageType {
  BROADCAST,
  ACK,
  FWD
}; /* Types of message that the application layer can send */

class PLMessage {
public:
  PLMessageType type;
  unsigned long sno;
  unsigned long orig_source;
  /* Single host_id that is interpreted based on the direction of the message.
   * For outgoing(incoming) messages it is the destination(sender) of the msg */
  unsigned long other;

  Message() : type(BROADCAST), sno(0), source(0), recvd_from(0) {}

  Message(MessageType type, unsigned long sno, unsigned long source)
      : type(type), sno(sno), source(source) {}

  std::string stringify() {
    std::string text;
    if (type == BROADCAST)
      text = "b " + std::to_string(sno);
    else if (type == ACK)
      text = "a " + std::to_string(sno);
    else
      text = "f " + std::to_string(sno) + " " + std::to_string(source);

    return text;
  }

  bool isValid() { return sno > 0; }
  bool isBroadCast() { return this->stringify()[0] == 'b'; }
  bool isAck() { return this->stringify()[0] == 'a'; }
  bool isFwd() { return this->stringify()[0] == 'f'; }
};

Message createBroadCastMsg(unsigned long sno);
Message createAckMsg(unsigned long sno);
Message createFwdMsg(unsigned long sno, unsigned long sender);
Message unmarshall(char *buf, unsigned long len, unsigned long host_id);

Message createBroadCastMsg(unsigned long sno) {
  Message msg;
  msg.sno = sno;
  return msg;
}
Message createAckMsg(unsigned long sno) {
  Message msg;
  msg.type = ACK;
  msg.sno = sno;
  return msg;
}
Message createFwdMsg(unsigned long sno, unsigned long source) {
  Message msg;
  msg.type = FWD;
  msg.sno = sno;
  msg.source = source;
  return msg;
}

Message unmarshall(char *buf, unsigned long len, unsigned long host_id) {
  Message msg;
  if (len == 0) {
    throw std::runtime_error("Unmarshalling null message" +
                             std::string(std::strerror(errno)));
  }
  std::string str(buf);
  std::string delim = " ";
  if (str.at(0) == 'b') {
    if (std::count(str.begin(), str.end(), delim[0]) != 1)
      throw std::runtime_error("Unmarshalling mangled broadcast" +
                               std::string(std::strerror(errno)));
    std::size_t found = str.find_first_of(delim);
    unsigned long sno = std::stoul(str.substr(found + 1), NULL, 0);
    msg = createBroadCastMsg(sno);

  } else if (str.at(0) == 'a') {
    if (std::count(str.begin(), str.end(), delim[0]) != 1)
      throw std::runtime_error("Unmarshalling mangled ack" +
                               std::string(std::strerror(errno)));
    std::size_t found = str.find_first_of(delim);
    unsigned long sno = std::stoul(str.substr(found + 1), NULL, 0);
    msg = createAckMsg(sno);

  } else if (str.at(0) == 'f') {
    if (std::count(str.begin(), str.end(), delim[0]) != 2)
      throw std::runtime_error("Unmarshalling mangled fwd" +
                               std::string(std::strerror(errno)));
    std::size_t found1 = str.find_first_of(delim);
    std::size_t found2 = str.substr(found1 + 1).find_first_of(delim);
    unsigned long sno =
        std::stoul(str.substr(found1 + 1, found2 - found1 - 1), NULL, 0);
    unsigned long sender_id = std::stoul(str.substr(found2 + 1), NULL, 0);
    msg = createFwdMsg(sno, sender_id);

  } else
    throw std::runtime_error("Unmarshalling mangled message" +
                             std::string(std::strerror(errno)));
  msg.recvd_from = host_id;
  return msg;
}