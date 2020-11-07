#pragma once
#include "parser.hpp"
#include <map>

bool compareSockAddr(sockaddr_in a, sockaddr_in b);

bool compareSockAddr(sockaddr_in a, sockaddr_in b) {
  return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

class Communicator {
private:
  unsigned long id;
  int server_sockfd;
  struct sockaddr_in server_addr;
  std::map<unsigned long, struct sockaddr_in> others;

public:
  Communicator(unsigned long id, std::vector<Parser::Host> hosts) : id(id) {

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

  void sendMessage(unsigned long msg_id, unsigned long host_id) {

    struct sockaddr_in cli_addr;

    if (others.find(host_id) == others.end()) {
      throw std::runtime_error("Tried to send message to non-existent process" +
                               std::string(std::strerror(errno)));
    }
    cli_addr = others[host_id];

    for (int i = 0; i < 5; i++) {

      std::string msg = std::to_string(msg_id);
      msg = "b " + msg;
      sendto(server_sockfd, reinterpret_cast<const char *>(msg.c_str()),
             strlen(msg.c_str()), MSG_CONFIRM,
             reinterpret_cast<struct sockaddr *>(&cli_addr), sizeof(cli_addr));
      std::cout << "Message with id " << msg_id << "sent to host " << host_id
                << std::endl;

      // unsigned long n;
      // unsigned int len;
      // char buffer[4];
      // n = recvfrom(server_sockfd, reinterpret_cast<char *>(buffer), 4,
      //              MSG_WAITALL, reinterpret_cast<struct sockaddr
      //              *>(&server_addr), &len);
      // std::cout << "Ack received\n";
      // if (n != 3) {
      //   throw std::runtime_error("Received ACK is mangled" +
      //                            std::string(std::strerror(errno)));
      // }
      // buffer[n] = '\0';
      // std::cout << buffer << std::endl;
    }
  }

  void recvMessage() {
    unsigned long n;
    unsigned int len;
    char buffer[100];
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));
    int x;
    std::cout << "Trying to receive messages\n";
    do {
      x = 0;
      n = recvfrom(server_sockfd, reinterpret_cast<char *>(buffer), 4,
                   MSG_WAITALL, reinterpret_cast<struct sockaddr *>(&cli_addr),
                   &len);
      buffer[n] = '\0';
      for (auto it : others) {
        if (compareSockAddr(it.second, cli_addr)) {
          std::cout << "Received message from process " << it.first << "\n";
        }
        x = 1;
        break;
      }
      std::cout << buffer << std::endl;
    } while (x == 0);
  }
};