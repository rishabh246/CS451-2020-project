#include <AppMessage.hpp>
#include <cerrno>
#include <clocale>
#include <stdlib.h>
#include <string>

enum LogMessageType {
  Broadcast,
  Delivery
}; /* Types of message that the application layer can send */

std::string stringify_log_source(unsigned long id);
std::string stringify_log_sno(unsigned long sno);
std::string stringify_log_type(LogMessageType type);

class LogMessage {
public:
  LogMessageType type;
  unsigned long source;
  unsigned long sno;

  LogMessage(LogMessageType type, unsigned long source, unsigned long sno)
      : type(type), source(source), sno(sno) {}

  std::string stringify() {
    if (type == Broadcast)
      return stringify_log_type(type) + " " + stringify_log_sno(sno);
    return stringify_log_type(type) + " " + stringify_log_source(source) + " " +
           stringify_log_sno(sno);
  }
};

std::string stringify_log_type(LogMessageType type) {
  if (type == Broadcast)
    return "b";
  return "d";
}

std::string stringify_log_source(unsigned long id) {
  std::stringstream s;
  s << std::setfill('0') << std::setw(3) << id;
  return s.str();
}

std::string stringify_log_sno(unsigned long sno) {
  std::stringstream s;
  s << std::setfill('0') << std::setw(6) << sno;
  return s.str();
}