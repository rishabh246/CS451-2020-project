#pragma once

#include "URB.hpp"

class FIFOBroadCast {
public:
  URB urb;
  unsigned long sno;
  unsigned long id;
  /* Likely to get quite big. Key is msg.source */
  SharedMapVec<unsigned long, unsigned long> pending;
  SharedQueue<AppMessage> outgoing;
  FIFOBroadCast() {}
  ~FIFOBroadCast() {}
};

/* Interface */
void FIFOBroadCast_init(FIFOBroadCast *fifobc, unsigned long host_id,
                        std::vector<Parser::Host> hosts,
                        std::vector<std::thread> *threads);
void FIFOBroadCast_send(FIFOBroadCast *fifobc);
AppMessage FIFOBroadCast_recv(FIFOBroadCast *fifobc);

/* Internal functions */
void fifobc_delivery(FIFOBroadCast *fifobc);

/* Definitions */

void FIFOBroadCast_init(FIFOBroadCast *fifobc, unsigned long host_id,
                        std::vector<Parser::Host> hosts,
                        std::vector<std::thread> *threads) {
  fifobc->id = host_id;
  fifobc->sno = 1;
  URB_init(&(fifobc->urb), host_id, hosts, threads);
  /* Launch background threads */
  threads->push_back(std::thread(fifobc_delivery, fifobc));
}

void FIFOBroadCast_send(FIFOBroadCast *fifobc) {
  AppMessage msg(fifobc->id, fifobc->sno);
  URB_send(&(fifobc->urb), msg);
  fifobc->sno++;
}

AppMessage FIFOBroadCast_recv(FIFOBroadCast *fifobc) {
  AppMessage msg = fifobc->outgoing.front();
  fifobc->outgoing.pop_front();
  return msg;
}

void fifobc_delivery(FIFOBroadCast *fifobc) {
  std::string identifier = "[FIFOBroadCast delivery thread] :";
  std::map<std::pair<unsigned long, unsigned long>, unsigned long>
      acked; // Hack to ensure std::less operator exists
  unsigned long num_processes = fifobc->urb.others.size() + 1;
  while (1) {
    AppMessage msg = BEB_recv(&(fifobc->urb));
    std::pair<unsigned long, unsigned long> msg_pair =
        std::make_pair(msg.source, msg.sno);

    /* Updating acked */
    if (acked.find(msg_pair) == acked.end())
      acked[msg_pair] = 0;
    acked[msg_pair]++;
    if (acked[msg_pair] > num_processes / 2)
      fifobc->outgoing.push_back(msg);

    /* Updating pending and broadcasting */
    if (!fifobc->pending.exists(msg.source, msg.sno)) {
      fifobc->pending.insert_item(msg.source, msg.sno);
      /* This must be done independent of ack status since you might be the only
       * correct process that has received everything */
      BEB_send(&(fifobc->urb), msg);
    }
  }
  std::cout << "BUG: Reached end of " << identifier << "\n";
}