#pragma once
#include <cerrno>
#include <clocale>
#include <cstring>
#include <iomanip>
#include <stdlib.h>
#include <string>

enum AppMessageType {
  BROADCAST,
  ACK,
  FWD
}; /* Types of message that the application layer can send */

#define IDStrLen 5   /* Host ID specified by 6 characters */
#define SNOStrLen 12 /* Serial number specified by 13 characters */
#define TypeStrLen 3 /* Type specified using 4 characters */

std::string stringify_host_ids(unsigned long id);
std::string stringify_sno(unsigned long sno);
std::string stringify_type(AppMessageType type);

unsigned long unmarshall_host_ids(std::string text);
unsigned long unmarshall_sno(std::string text);
AppMessageType unmarshall_type(std::string text);

class AppMessage {
public:
  AppMessageType type;
  unsigned long sno;
  unsigned long orig_source;
  unsigned long sender;
  unsigned long receiver;

  AppMessage()
      : type(BROADCAST), sno(0), orig_source(0), sender(0), receiver(0) {}

  AppMessage(AppMessageType type, unsigned long sno, unsigned long orig_source,
             unsigned long sender, unsigned long receiver)
      : type(type), sno(sno), orig_source(orig_source), sender(sender),
        receiver(receiver) {}

  AppMessage(const AppMessage &msg)
      : type(msg.type), sno(msg.sno), orig_source(msg.orig_source),
        sender(msg.sender), receiver(msg.receiver) {}

  std::string stringify() {
    return stringify_host_ids(sender) + stringify_type(type) +
           stringify_sno(sno) + stringify_host_ids(orig_source);
  }
};

std::string stringify_host_ids(unsigned long id) {
  std::stringstream s;
  s << "[" << std::setfill('0') << std::setw(3) << id << ']';
  return s.str();
}

std::string stringify_sno(unsigned long sno) {
  std::stringstream s;
  s << "[" << std::setfill('0') << std::setw(3) << sno << ']';
  return s.str();
}

std::string stringify_type(AppMessageType type) {
  std::string text;
  if (type == BROADCAST)
    text = "[B]";
  else if (type == ACK)
    text = "[A]";
  else
    text = "[F]";
  return text;
}

unsigned long unmarshall_host_ids(std::string text) {
  text = text.substr(1, IDStrLen - 1);
  return std::stoul(text);
}
unsigned long unmarshall_sno(std::string text) {
  text = text.substr(1, SNOStrLen - 1);
  return std::stoul(text);
}
AppMessageType unmarshall_type(std::string text) {
  text = text.substr(1, TypeStrLen - 1);
  return static_cast<AppMessageType>(std::stoul(text));
}

AppMessage AppUnmarshall(char *buf, unsigned long len);
AppMessage AppUnmarshall(char *buf, unsigned long len) {
  if (len != IDStrLen + TypeStrLen + SNOStrLen + IDStrLen)
  /* TODO: Need to guard against more complex errors */ {
    throw std::runtime_error("Unmarshalling mangled AppMessage" +
                             std::string(std::strerror(errno)));
  }
  AppMessage msg;
  AppMessageType type;
  unsigned long sno, orig_source, sender;
  std::string str(buf);
  sender = unmarshall_host_ids(str.substr(0, IDStrLen));
  type = unmarshall_type(str.substr(IDStrLen, TypeStrLen));
  sno = unmarshall_sno(str.substr(IDStrLen + TypeStrLen, SNOStrLen));
  orig_source = unmarshall_host_ids(
      str.substr(IDStrLen + TypeStrLen + SNOStrLen, IDStrLen));
  return AppMessage(type, sno, orig_source, sender,
                    0); /* Not ideal, but this is set by the FLL */
}

// AppMessage createBroadCastMsg(unsigned long sno);
// AppMessage createAckMsg(unsigned long sno);
// AppMessage createFwdMsg(unsigned long sno, unsigned long sender);

// AppMessage createBroadCastMsg(unsigned long sno) {
//   AppMessage msg;
//   msg.sno = sno;
//   return msg;
// }
// AppMessage createAckMsg(unsigned long sno) {
//   AppMessage msg;
//   msg.type = ACK;
//   msg.sno = sno;
//   return msg;
// }
// AppMessage createFwdMsg(unsigned long sno, unsigned long source) {
//   AppMessage msg;
//   msg.type = FWD;
//   msg.sno = sno;
//   msg.source = source;
//   return msg;
// }
