#pragma once

#include "NetworkMessage.hpp"
#include "PerfectLink.hpp"
#include "data-structures.hpp"

class BEB {
public:
  unsigned long id;
  PerfectLink PL;
  std::vector<unsigned long> others;
  BEB() {}
  ~BEB() {}
};

/* Interface */
void BEB_init(BEB *beb, unsigned long host_id, std::vector<Parser::Host> hosts,
              std::vector<std::thread> *threads);
void BEB_send(BEB *beb, AppMessage msg);
AppMessage BEB_recv(BEB *beb);

void BEB_init(BEB *beb, unsigned long host_id, std::vector<Parser::Host> hosts,
              std::vector<std::thread> *threads) {
  beb->id = host_id;
  for (auto &host : hosts) {
    if (host.id != beb->id)
      beb->others.push_back(host.id);
  }
  PL_init(&(beb->PL), host_id, hosts, threads);
}

void BEB_send(BEB *beb, AppMessage msg) {
  NetworkMessage new_msg(msg, beb->id, 0);
  for (auto it : beb->others) {
    new_msg.receiver = it;
    PL_send(&(beb->PL), new_msg);
  }
}
AppMessage BEB_recv(BEB *beb) {
  NetworkMessage msg = PL_recv(&(beb->PL));
  return msg.app_msg;
}