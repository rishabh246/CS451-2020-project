#include <chrono>
#include <iostream>
#include <thread>

#include "barrier.hpp"
#include "communicator.hpp"
#include "hello.h"
#include "parser.hpp"
#include <signal.h>

Communicator communicator;
unsigned long num_messages;

static void test(Message *msg) {
  std::cout << "Received message " << msg->stringify() << "from process "
            << msg->recvd_from << "\n";
}

static void stop(int) {
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // write/flush output file if necessary
  std::cout << "Writing output.\n";

  // exit directly from signal handler
  exit(0);
}

int main(int argc, char **argv) {
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // `true` means that a config file is required.
  // Call with `false` if no config file is necessary.
  bool requireConfig = true;

  Parser parser(argc, argv, requireConfig);
  parser.parse();

  hello();
  std::cout << std::endl;

  std::cout << "My PID: " << getpid() << "\n";
  std::cout << "Use `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "My ID: " << parser.id() << "\n\n";

  std::cout << "Path to hosts:\n";
  std::cout << "==============\n";
  std::cout << parser.hostsPath() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts) {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readable Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readable Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "Barrier:\n";
  std::cout << "========\n";
  auto barrier = parser.barrier();
  std::cout << "Human-readable IP: " << barrier.ipReadable() << "\n";
  std::cout << "Machine-readable IP: " << barrier.ip << "\n";
  std::cout << "Human-readable Port: " << barrier.portReadable() << "\n";
  std::cout << "Machine-readable Port: " << barrier.port << "\n";
  std::cout << "\n";

  std::cout << "Signal:\n";
  std::cout << "========\n";
  auto signal = parser.signal();
  std::cout << "Human-readable IP: " << signal.ipReadable() << "\n";
  std::cout << "Machine-readable IP: " << signal.ip << "\n";
  std::cout << "Human-readable Port: " << signal.portReadable() << "\n";
  std::cout << "Machine-readable Port: " << signal.port << "\n";
  std::cout << "\n";

  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.outputPath() << "\n\n";

  if (requireConfig) {
    std::cout << "Path to config:\n";
    std::cout << "===============\n";
    std::cout << parser.configPath() << "\n\n";
    std::cout << "Config Data:\n";
    std::cout << "===============\n";
    std::cout << "Num messages:" << parser.numMessages() << "\n\n";
  }

  std::cout << "Doing some initialization...\n\n";

  num_messages = parser.numMessages();
  Coordinator coordinator(parser.id(), barrier, signal);
  communicator.init(parser.id(), hosts);

  std::cout << "Waiting for all processes to finish initialization\n\n";
  coordinator.waitOnBarrier();

  std::cout << "Broadcasting messages...\n\n";

  unsigned long other, sno = 1, recvno = 0;
  Message msg[5], recv[5];

  if (parser.id() == 1)
    other = 2;
  else
    other = 1;

  std::thread threads[5];

  while (1) {
    if (sno < 6) {
      msg[sno - 1] = createBroadCastMsg(sno);
      communicator.sendMessage(msg[sno - 1], other);
    }
    recv[recvno] = communicator.recvMessage();
    if (recv[recvno].isValid()) {
      threads[recvno] = std::thread(test, &recv[recvno]);
      recvno++;
    }
    sno++;
  }

  std::cout << "Signaling end of broadcasting messages\n\n";
  coordinator.finishedBroadcasting();

  for (int i = 0; i <= 4; i++) {
    threads[i].join();
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(60));
  }

  return 0;
}
