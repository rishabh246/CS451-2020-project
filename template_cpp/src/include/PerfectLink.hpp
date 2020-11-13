#include "FairLossLink.hpp"
#include "PLMessage.hpp"
#include "data-structures.hpp"
#include <assert.h>

/* Need to add no duplication property */
class PerfectLink {
public:
  FairLossLink FLL;
  SharedQueue<NetworkMessage> outgoing;
  SharedMapVec<unsigned long, NetworkMessage>
      unacked_messages;             // Key is destination of msg
  std::string identifier = "[PL]:"; // For debugging
  PerfectLink() {}
  ~PerfectLink() {}
};
/* Interface */
void PL_init(PerfectLink *link, unsigned long host_id,
             std::vector<Parser::Host> hosts,
             std::vector<std::thread> *threads);

void PL_send(PerfectLink *link, NetworkMessage msg);
NetworkMessage PL_recv(PerfectLink *link);

/* Internal functions */
void retransmit(PerfectLink *link);
void delivery(PerfectLink *link);

/* Definitions */
void PL_init(PerfectLink *link, unsigned long host_id,
             std::vector<Parser::Host> hosts,
             std::vector<std::thread> *threads) {
  FLL_init(&(link->FLL), host_id, hosts, threads);
  link->outgoing.debug_flag = 0;
  /* Launch background threads */
  threads->push_back(std::thread(retransmit, link));
  threads->push_back(std::thread(delivery, link));
}

void PL_send(PerfectLink *link, NetworkMessage msg) {
  PLMessage pl_msg = PLMessage(TX, msg);
  FLL_send(&(link->FLL), pl_msg);
  link->unacked_messages.insert_item(msg.receiver, msg);
  // std::cout << "[PL Sender]: " << pl_msg.msg.stringify() << " to host "
  //           << pl_msg.msg.receiver << "\n";
}

NetworkMessage PL_recv(PerfectLink *link) {
  NetworkMessage msg = link->outgoing.front();
  link->outgoing.pop_front();
  // std::cout << "[PL Receiver]: " << msg.stringify() << " \n";
  return msg;
}

void retransmit(PerfectLink *link) {
  std::string identifier = "[Retransmission thread]: ";
  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::map<unsigned long, std::vector<NetworkMessage>> unacked =
        link->unacked_messages.get_copy();
    for (auto dest : unacked) {
      for (auto msg : dest.second) {
        PLMessage pl_msg = PLMessage(TX, msg);
        FLL_send(&(link->FLL), pl_msg);
        // std::cout << "Re-sending message " << pl_msg.msg.stringify()
        //           << " to host " << pl_msg.msg.receiver << "\n";
      }
    }
  }
}
void delivery(PerfectLink *link) {
  std::string identifier = "[PL delivery thread] :";
  /* Map of already delivered messages to layer above.
  Stored as hash map and not vector to ensure fast lookups*/
  std::map<unsigned long, std::vector<NetworkMessage>> delivered;
  while (1) {
    PLMessage msg = FLL_recv(&(link->FLL));
    unsigned long recvd_from = msg.msg.sender;

    if (msg.type == TX) {
      // std::cout << identifier << msg.msg.stringify() << " from host "
      //           << msg.msg.sender << "\n";
      /* Ensure no duplication */
      if (delivered.find(recvd_from) == delivered.end()) {
        delivered[recvd_from] = std::vector<NetworkMessage>{msg.msg};
        link->outgoing.push_back(msg.msg);
        // std::cout << identifier << msg.msg.stringify() << " from host "
        //           << msg.msg.sender << "\n";
      } else {
        auto it = delivered[recvd_from].begin();
        for (; it != delivered[recvd_from].end(); it++) {
          if (msg.msg == *it)
            break;
        }
        if (it == delivered[recvd_from].end()) {
          delivered[recvd_from].push_back(msg.msg);
          link->outgoing.push_back(msg.msg);
          // std::cout << identifier << msg.msg.stringify() << " from host "
          //           << msg.msg.sender << "\n";
        }
      }
    } else {
      /* Remove message from list of unacked messages */
      /* Need to flip sender and receiver before comparing */
      unsigned long temp = msg.msg.sender;
      msg.msg.sender = msg.msg.receiver;
      msg.msg.receiver = temp;
      bool temp1 = link->unacked_messages.remove(recvd_from, msg.msg);
    }
  }
}
