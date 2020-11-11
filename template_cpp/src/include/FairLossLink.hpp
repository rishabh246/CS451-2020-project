#pragma once
#include "PLMessage.hpp"
#include "data-structures.hpp"
#include "debug.hpp"
#include "parser.hpp" /* TODO: Not great layering */
#include "socket-helpers.hpp"

#include <map>
#define MAX_MSG_SIZE 100 /* TODO check if this is enough */

class FairLossLink {
public:
  unsigned long host_id;
  int sockfd;
  struct sockaddr_in socket_addr;
  std::map<unsigned long, struct sockaddr_in>
      others; /* Bad layering. Also read only */
  SharedQueue<PLMessage> incoming;
  SharedQueue<PLMessage> outgoing;
  std::string identifier = "[FLL]:"; // For debugging
  FairLossLink() {}
  ~FairLossLink() {}
};
/* Interface */
void FLL_init(FairLossLink *link, unsigned long host_id,
              std::vector<Parser::Host> hosts,
              std::vector<std::thread> *threads);

void FLL_send(FairLossLink *link, PLMessage msg);
PLMessage FLL_recv(FairLossLink *link);

/* Internal functions */
void sender(FairLossLink *link);
void receiver(FairLossLink *link);

/* Definitions */

void FLL_init(FairLossLink *link, unsigned long host_id,
              std::vector<Parser::Host> hosts,
              std::vector<std::thread> *threads) {

  link->host_id = host_id;
  struct sockaddr_in addr;
  for (auto &host : hosts) {

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = host.ip;
    addr.sin_port = host.port;

    if (host.id == link->host_id) {
      int fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (fd < 0) {
        throw std::runtime_error("Could not create the process socket: " +
                                 std::string(std::strerror(errno)));
      }

      if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) <
          0) {
        throw std::runtime_error("Could not bind the process socket: " +
                                 std::string(std::strerror(errno)));
      }
      link->sockfd = fd;
      memset(&link->socket_addr, 0, sizeof(link->socket_addr));
      link->socket_addr.sin_family = AF_INET;
      link->socket_addr.sin_addr.s_addr = host.ip;
      link->socket_addr.sin_port = host.port;
    }

    else {
      if (link->others.find(host.id) != link->others.end()) {
        throw std::runtime_error("Hosts file is broken due to duplicate ids: " +
                                 std::string(std::strerror(errno)));
      }
      link->others[host.id] = addr;
    }
  }
  /* Launch background threads */
  threads->push_back(std::thread(sender, link));
  threads->push_back(std::thread(receiver, link));
}

void FLL_send(FairLossLink *link, PLMessage msg) {
  link->incoming.push_back(msg);
}

PLMessage FLL_recv(FairLossLink *link) {
  PLMessage msg = link->outgoing.front();
  link->outgoing.pop_front();
  return msg;
}

void sender(FairLossLink *link) {

  std::string identifier = "[FLL Sender Thread]:";
  struct sockaddr_in cli_addr;
  memset(&cli_addr, 0, sizeof(cli_addr));

  while (1) {

    PLMessage msg = link->incoming.front();
    link->incoming.pop_front();
    std::string identifier = "[FLL Sender Thread]:";

    if (link->others.find(msg.msg.receiver) == link->others.end()) {
      throw std::runtime_error(
          "Tried to send PLMessage to non-existent process " +
          std::string(std::strerror(errno)));
    }
    cli_addr = link->others[msg.msg.receiver];

    std::string msgstr = msg.stringify();
    ssize_t retcode = sendto(
        link->sockfd, reinterpret_cast<const char *>(msgstr.c_str()),
        strlen(msgstr.c_str()), MSG_CONFIRM,
        reinterpret_cast<struct sockaddr *>(&cli_addr), sizeof(cli_addr));
    if (retcode < 0)
      throw std::runtime_error("SendPLMessage failure due to " +
                               std::string(std::strerror(errno)));
    // std::cout << identifier << "PLMessage " << msgstr << " sent to host "
    //           << msg.msg.receiver << std::endl;
  }
  std::cout << "BUG: Reached end of " << identifier << "\n";
}

void receiver(FairLossLink *link) {
  std::string identifier = "[FLL Receiver Thread]:";
  unsigned long n;
  unsigned int len;
  char buffer[MAX_MSG_SIZE];
  struct sockaddr_in cli_addr;
  memset(&cli_addr, 0, sizeof(cli_addr));

  while (1) {

    n = recvfrom(link->sockfd, reinterpret_cast<char *>(buffer), MAX_MSG_SIZE,
                 MSG_WAITALL, reinterpret_cast<struct sockaddr *>(&cli_addr),
                 &len);
    if (n > 0) {
      buffer[n] = '\0';
      for (auto it : link->others) {
        if (compareSockAddr(it.second, cli_addr)) {
          /* An addition check that a useful PLMessage has been received */
          std::string s(buffer);
          PLMessage msg = PLUnmarshall(buffer, n);
          msg.msg.receiver = link->host_id;
          link->outgoing.push_back(msg);
          if (msg.type == TX) {
            /* Don't send. Just add it to the incoming queue*/
            PLMessage ack = createAck(msg);
            ack.msg.sender = link->host_id;
            ack.msg.receiver = it.first;
            link->incoming.push_back(ack);
          }
          break;
        }
      }
    }
  }
  std::cout << "BUG: Reached end of " << identifier << "\n";
}