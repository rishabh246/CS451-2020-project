#pragma once

#include "URB.hpp"
#define MAX_NUM_PROCESSES 128
class FIFOBroadcast {
public:
  URB urb;
  unsigned long sno;
  unsigned long id;
  SharedQueue<AppMessage> outgoing;
  FIFOBroadcast() {}
  ~FIFOBroadcast() {}
};

/* Interface */
void FIFOBroadcast_init(FIFOBroadcast *fifobc, unsigned long host_id,
                        std::vector<Parser::Host> hosts,
                        std::vector<std::thread> *threads);
AppMessage FIFOBroadcast_send(FIFOBroadcast *fifobc);
AppMessage FIFOBroadcast_recv(FIFOBroadcast *fifobc);

/* Internal functions */
void fifobc_delivery(FIFOBroadcast *fifobc);

/* Definitions */

void FIFOBroadcast_init(FIFOBroadcast *fifobc, unsigned long host_id,
                        std::vector<Parser::Host> hosts,
                        std::vector<std::thread> *threads) {
  fifobc->id = host_id;
  fifobc->sno = 1;
  URB_init(&(fifobc->urb), host_id, hosts, threads);
  /* Launch background threads */
  threads->push_back(std::thread(fifobc_delivery, fifobc));
}

AppMessage FIFOBroadcast_send(FIFOBroadcast *fifobc) {
  AppMessage msg(fifobc->id, fifobc->sno);
  URB_send(&(fifobc->urb), msg);
  fifobc->sno++;
  return msg;
}

AppMessage FIFOBroadcast_recv(FIFOBroadcast *fifobc) {
  AppMessage msg = fifobc->outgoing.pop_front();
  return msg;
}

void fifobc_delivery(FIFOBroadcast *fifobc) {
  std::string identifier = "[FIFOBroadcast delivery thread] :";
  unsigned long num_processes = fifobc->urb.beb.others.size() + 1;
  unsigned long
      next_delivered[MAX_NUM_PROCESSES + 1]; /* We ignore the zeroth element */
  for (int i = 0; i <= MAX_NUM_PROCESSES; i++)
    next_delivered[i] = 1;
  /* Key is msg.source, value is priority_queue of snos */
  std::map<unsigned long,
           std::priority_queue<unsigned long, std::vector<unsigned long>,
                               std::greater<unsigned long>>>
      pending;
  while (1) {
    AppMessage msg = URB_recv(&(fifobc->urb));
    // std::cout << identifier << msg.stringify() << "\n" << std::flush;
    if (pending.find(msg.source) == pending.end()) {
      std::priority_queue<unsigned long, std::vector<unsigned long>,
                          std::greater<unsigned long>>
          temp;
      pending[msg.source] = temp;
    }
    pending[msg.source].push(msg.sno);
    while (pending[msg.source].top() == next_delivered[msg.source]) {
      AppMessage final_msg(msg.source, pending[msg.source].top());
      fifobc->outgoing.push_back(final_msg);
      pending[msg.source].pop();
      next_delivered[msg.source]++;
    }
  }
  std::cout << "BUG: Reached end of " << identifier << "\n";
}