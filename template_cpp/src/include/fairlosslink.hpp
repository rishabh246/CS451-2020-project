#pragma once
#include "message.hpp"
#include "parser.hpp" /* TODO: Not great layering */
#include "shared-data.hpp"
#include "socket_helpers.hpp"

#include <map>
#define MAX_MSG_SIZE 100 /* TODO check if this is enough */

class FairLossLink {
public:
  unsigned long id;
  int sockfd;
  struct sockaddr_in socket_addr;
  std::map<unsigned long, struct sockaddr_in>
      others; /* Bad layering. Also read only */
  SharedQueue<Message> delivery;
  std::string identifier = "[FLL]:"; // For debugging
  FairLossLink() {}
  ~FairLossLink() {}
};
/* Interface */
void FLL_init(FairLossLink *link, unsigned long host_id,
              std::vector<Parser::Host> hosts, std::thread *threads,
              uint *thread_no);

void FLL_send(FairLossLink *link, Message msg, unsigned long host_id);
Message FLL_recv(FairLossLink *link);

/* Internal functions */
void socket_listen(FairLossLink *link);

/* Definitions */

void FLL_init(FairLossLink *link, unsigned long host_id,
              std::vector<Parser::Host> hosts, std::thread *threads,
              uint *thread_no) {

  link->id = host_id;
  struct sockaddr_in addr;
  for (auto &host : hosts) {

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = host.ip;
    addr.sin_port = host.port;

    if (host.id == link->id) {
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
  threads[*thread_no] = std::thread(socket_listen, link);
  *thread_no++;
}

void FLL_send(FairLossLink *link, Message msg, unsigned long host_id) {

  struct sockaddr_in cli_addr;
  if (link->others.find(host_id) == link->others.end()) {
    throw std::runtime_error("Tried to send message to non-existent process" +
                             std::string(std::strerror(errno)));
  }
  cli_addr = link->others[host_id];

  std::string msgstr = msg.stringify();
  ssize_t retcode =
      sendto(link->sockfd, reinterpret_cast<const char *>(msgstr.c_str()),
             strlen(msgstr.c_str()), MSG_CONFIRM,
             reinterpret_cast<struct sockaddr *>(&cli_addr), sizeof(cli_addr));
  if (retcode < 0)
    throw std::runtime_error("SendMessage failure due to " +
                             std::string(std::strerror(errno)));
  std::cout << link->identifier << "Message " << msgstr << " sent to host "
            << host_id << std::endl;
}

Message FLL_recv(FairLossLink *link) {
  Message msg = link->delivery.front();
  link->delivery.pop_front();
  return msg;
}

void socket_listen(FairLossLink *link) {
  std::string identifier = "[FLL Listener Thread]:";
  unsigned long n;
  unsigned int len;
  char buffer[MAX_MSG_SIZE];
  Message msg;
  struct sockaddr_in cli_addr;
  memset(&cli_addr, 0, sizeof(cli_addr));

  while (1) {

    n = recvfrom(link->sockfd, reinterpret_cast<char *>(buffer), 4, MSG_WAITALL,
                 reinterpret_cast<struct sockaddr *>(&cli_addr), &len);
    if (n > 0) {
      buffer[n] = '\0';
      for (auto it : link->others) {
        if (compareSockAddr(it.second, cli_addr)) {
          /* Useful message received */
          msg = unmarshall(buffer, n, it.first);
          std::cout << identifier << "Received message " << msg.stringify()
                    << "from process " << msg.recvd_from << "\n";
          link->delivery.push_back(msg);
          if (!msg.isAck()) {
            std::string ackstr = createAckMsg(msg.sno).stringify();
            ssize_t retcode = sendto(
                link->sockfd, reinterpret_cast<const char *>(ackstr.c_str()),
                strlen(ackstr.c_str()), MSG_CONFIRM,
                reinterpret_cast<struct sockaddr *>(&cli_addr),
                sizeof(cli_addr));
            if (retcode < 0)
              throw std::runtime_error("SendAck failure due to " +
                                       std::string(std::strerror(errno)));
            std::cout << identifier << "Ack " << ackstr << " sent to host "
                      << msg.recvd_from << std::endl;
          }
          break;
        }
      }
    }
  }
}