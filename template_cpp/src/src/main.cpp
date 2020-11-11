#include <chrono>
#include <iostream>
#include <thread>

#include "FairLossLink.hpp"
#include "barrier.hpp"
#include "hello.h"
#include "parser.hpp"
#include <signal.h>

/* Globals */

FairLossLink FLL;
unsigned long num_messages;

#define MAX_THREAD_COUNT 10
std::vector<std::thread> threads;

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
  FLL_init(&FLL, parser.id(), parser.hosts(), &threads);
  std::cout << threads.size() << "\n";
  std::cout << "Waiting for all processes to finish initialization\n\n";
  coordinator.waitOnBarrier();

  std::cout << "Broadcasting messages...\n\n";

  unsigned long other, sno = 20;

  if (parser.id() == 1)
    other = 2;
  else
    other = 1;

  AppMessage app_msg;
  app_msg.sender = parser.id();
  app_msg.receiver = other;
  app_msg.orig_source = app_msg.sender;

  PLMessage pl_msg(TX, app_msg);

  do {
    pl_msg.msg.sno = sno++;
    FLL_send(&FLL, pl_msg);
  } while (sno < 20 + num_messages);

  std::cout << "Signaling end of broadcasting messages\n\n";
  coordinator.finishedBroadcasting();

  for (uint i = 0; i <= threads.size(); i++) {
    threads[i].join();
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(60));
  }

  return 0;
}
