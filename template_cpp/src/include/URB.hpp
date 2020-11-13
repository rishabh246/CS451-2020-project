#pragma once

#include "AppMessage.hpp"
#include "BEB.hpp"
#include "data-structures.hpp"

class URB {
public:
  BEB beb;
  /* Likely to get quite big. Key is msg.source */
  SharedMapVec<unsigned long, unsigned long> pending;
  SharedQueue<AppMessage> outgoing;
  URB() {}
  ~URB() {}
};

/* Interface */
void URB_init(URB *urb, unsigned long host_id, std::vector<Parser::Host> hosts,
              std::vector<std::thread> *threads);
void URB_send(URB *urb, AppMessage msg);
AppMessage URB_recv(URB *urb);

/* Internal functions */
void urb_delivery(URB *urb);

/* Definitions */

void URB_init(URB *urb, unsigned long host_id, std::vector<Parser::Host> hosts,
              std::vector<std::thread> *threads) {
  BEB_init(&(urb->beb), host_id, hosts, threads);
  /* Launch background threads */
  threads->push_back(std::thread(urb_delivery, urb));
}

void URB_send(URB *urb, AppMessage msg) {
  urb->pending.insert_item(msg.source, msg.sno);
  BEB_send(&(urb->beb), msg);
}

AppMessage URB_recv(URB *urb) {
  AppMessage msg = urb->outgoing.pop_front();
  return msg;
}

void urb_delivery(URB *urb) {
  std::string identifier = "[URB delivery thread] :";
  std::map<std::pair<unsigned long, unsigned long>, unsigned long>
      acked; // Hack to ensure std::less operator exists
  unsigned long num_processes = urb->beb.others.size() + 1;
  while (1) {
    AppMessage msg = BEB_recv(&(urb->beb));
    std::pair<unsigned long, unsigned long> msg_pair =
        std::make_pair(msg.source, msg.sno);

    /* Updating acked */
    // std::cout << identifier << msg.stringify() << "\n" << std::flush;
    if (acked.find(msg_pair) == acked.end())
      acked[msg_pair] = 0;
    if (acked[msg_pair] < num_processes) {
      /* This message has not been delivered yet */
      acked[msg_pair]++;
      if (acked[msg_pair] > (num_processes / 2)) {
        urb->outgoing.push_back(msg);
        acked[msg_pair] = num_processes;
        // std::cout << identifier << msg.stringify() << "\n";
      }
      /* Updating pending and broadcasting */
      if (!urb->pending.exists(msg.source, msg.sno)) {
        urb->pending.insert_item(msg.source, msg.sno);
        /* This must be done independent of ack status since you might be the
         * only correct process that has received everything */
        BEB_send(&(urb->beb), msg);
      }
    }
  }
  std::cout << "BUG: Reached end of " << identifier << "\n";
}