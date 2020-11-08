#pragma once
#include "message.hpp"
#include "parser.hpp" /* TODO: Not great layering */

#include <map>
#define MAX_MSG_SIZE 100 /* TODO check if this is enough */

bool compareSockAddr(sockaddr_in a, sockaddr_in b);

bool compareSockAddr(sockaddr_in a, sockaddr_in b) {
  return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

class FairLossLink {
private:
  unsigned long id;
  int server_sockfd;
  struct sockaddr_in server_addr;
  std::map<unsigned long, struct sockaddr_in> others;

public:
  Communicator() {}
  void init(unsigned long host_id, std::vector<Parser::Host> hosts) {

    id = host_id;
    struct sockaddr_in addr;
    for (auto &host : hosts) {

      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = host.ip;
      addr.sin_port = host.port;

      if (host.id == id) {
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
        server_sockfd = fd;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = host.ip;
        server_addr.sin_port = host.port;
      }

      else {
        if (others.find(host.id) != others.end()) {
          throw std::runtime_error(
              "Hosts file is broken due to duplicate ids: " +
              std::string(std::strerror(errno)));
        }
        others[host.id] = addr;
      }
    }
  }

  void sendMessage(Message msg, unsigned long host_id) {

    struct sockaddr_in cli_addr;
    if (others.find(host_id) == others.end()) {
      throw std::runtime_error("Tried to send message to non-existent process" +
                               std::string(std::strerror(errno)));
    }
    cli_addr = others[host_id];

    std::string msgstr = msg.stringify();
    ssize_t retcode = sendto(
        server_sockfd, reinterpret_cast<const char *>(msgstr.c_str()),
        strlen(msgstr.c_str()), MSG_CONFIRM,
        reinterpret_cast<struct sockaddr *>(&cli_addr), sizeof(cli_addr));
    if (retcode < 0)
      throw std::runtime_error("SendMessage failure due to " +
                               std::string(std::strerror(errno)));
    std::cout << "Message " << msgstr << " sent to host " << host_id
              << std::endl;
  }

  Message recvMessage() {
    unsigned long n;
    unsigned int len;
    char buffer[MAX_MSG_SIZE];
    Message msg;
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));

    n = recvfrom(server_sockfd, reinterpret_cast<char *>(buffer), 4,
                 MSG_WAITALL, reinterpret_cast<struct sockaddr *>(&cli_addr),
                 &len);
    if (n > 0) {
      buffer[n] = '\0';
      for (auto it : others) {
        if (compareSockAddr(it.second, cli_addr)) {
          msg = unmarshall(buffer, n, it.first);
          break;
        }
      }
    }

    return msg;
  }
};