#pragma once
#include "State.h"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
class Receiver {
 std::mutex mutex;
 std::vector<std::weak_ptr<State>> instances;
 std::thread thread;
 std::atomic<bool> running{true};
 int port;
 void run();
public:
 explicit Receiver(int defaultPort);
 ~Receiver();
 void add(const std::shared_ptr<State>& s);
 void remove(const std::shared_ptr<State>& s);
};
std::shared_ptr<Receiver> acquireReceiver();
