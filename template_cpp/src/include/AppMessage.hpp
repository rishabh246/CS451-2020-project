#pragma once
#include "debug.hpp"
#include <cerrno>
#include <clocale>
#include <cstring>
#include <iomanip>
#include <stdlib.h>
#include <string>

#define IDStrLen 5  /* Host ID specified by 5 characters */
#define SNOStrLen 8 /* Serial number specified by 8 characters */

std::string stringify_host_ids(unsigned long id);
std::string stringify_sno(unsigned long sno);

unsigned long unmarshall_host_ids(std::string text);
unsigned long unmarshall_sno(std::string text);

class AppMessage {
public:
  unsigned long source;
  unsigned long sno;

  AppMessage() : source(0), sno(0) {}

  AppMessage(unsigned long source, unsigned long sno)
      : source(source), sno(sno) {}

  AppMessage(const AppMessage &msg) : source(msg.source), sno(msg.sno) {}

  bool operator==(const AppMessage &other) {
    return (this->source == other.source && this->sno == other.sno);
  }

  std::string stringify() {
    return stringify_host_ids(source) + stringify_sno(sno);
  }
};

std::string stringify_host_ids(unsigned long id) {
  std::stringstream s;
  s << "[" << std::setfill('0') << std::setw(3) << id << ']';
  return s.str();
}

std::string stringify_sno(unsigned long sno) {
  std::stringstream s;
  s << "[" << std::setfill('0') << std::setw(6) << sno << ']';
  return s.str();
}

unsigned long unmarshall_host_ids(std::string text) { return std::stoul(text); }
unsigned long unmarshall_sno(std::string text) { return std::stoul(text); }

AppMessage AppUnmarshall(char *buf, unsigned long len);
AppMessage AppUnmarshall(char *buf, unsigned long len) {
  if (len < IDStrLen + SNOStrLen)
  /* TODO: Need to guard against more complex errors */ {
    LOG(len);
    throw std::runtime_error("Unmarshalling mangled AppMessage" +
                             std::string(std::strerror(errno)));
  }
  AppMessage msg;
  unsigned long sno, source;
  std::string str(buf);
  source = unmarshall_host_ids(str.substr(1, IDStrLen - 2));
  sno = unmarshall_sno(str.substr(IDStrLen + 1, SNOStrLen - 2));
  return AppMessage(source, sno);
}