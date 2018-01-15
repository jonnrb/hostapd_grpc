#include <condition_variable>
#include <csignal>
#include <mutex>
#include <string>
#include <thread>

#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "server/hostapd_control_impl.h"
#include "server/metrics.h"

namespace {
std::condition_variable interrupt{};
std::mutex mu{};
bool is_interrupt{false};
}  // namespace

void handler(int _) {
  {
    std::lock_guard<std::mutex> l(mu);
    is_interrupt = true;
  }
  interrupt.notify_all();
}

int main(int argc, char** argv) {
  const std::string server_address{"0.0.0.0:8080"};
  const std::string metrics_address{"0.0.0.0:9090"};
  const std::string hostapd_control_dir{"/hostapd_control"};
  const std::string hostapd_client_dir{"/hostapd_clients"};

  std::unique_ptr<hostapd::SocketManager> socket_manager{
      new hostapd::SocketManager(5, hostapd_control_dir, hostapd_client_dir)};
  hostapd::HostapdControlImpl service{std::move(socket_manager)};

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  auto server = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << std::endl;

  prometheus::Exposer exposer{metrics_address};
  std::cout << "Metrics listening on " << metrics_address << std::endl;

  auto registry = std::make_shared<prometheus::Registry>();
  exposer.RegisterCollectable(registry);
  auto metrics_collector = std::thread([registry, &service]() {
    hostapd::Metrics metrics(registry);

    std::unique_lock<std::mutex> l(mu);
    while (!is_interrupt) {
      auto status = interrupt.wait_for(l, std::chrono::seconds{5});
      if (status == std::cv_status::timeout) {
        metrics.Scrape(&service);
      }
    }
  });

  signal(SIGINT, handler);
  auto interrupt_waiter = std::thread([&server]() {
    std::unique_lock<std::mutex> l{mu};
    while (!is_interrupt) interrupt.wait(l);
    server->Shutdown();
  });

  server->Wait();
  interrupt_waiter.join();
  metrics_collector.join();
  std::cout << "Server shut down" << std::endl;
}
