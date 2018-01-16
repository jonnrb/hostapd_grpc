#include <condition_variable>
#include <csignal>
#include <mutex>
#include <string>
#include <thread>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "server/hostapd_control_impl.h"
#include "server/metrics.h"

DEFINE_string(hostapd_grpc_addr, "0.0.0.0:8080", "Address gRPC will bind");
DEFINE_string(hostapd_metrics_addr, "0.0.0.0:9090",
              "Address Prometheus metrics will bind");
DEFINE_string(hostapd_control_dir, "/var/run/hostapd",
              "Path specified by ctrl_interface hostapd option");
DEFINE_string(hostapd_client_dir, "/var/run/hostapd_grpc",
              "Path to place client sockets in; hostapd must be able to read "
              "the same directory at this path!");
DEFINE_uint64(hostapd_metrics_scrape_interval_ms, 5000,
              "How often to scrape metrics from hostapd in milliseconds");

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
  google::InitGoogleLogging(argv[0]);

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::unique_ptr<hostapd::SocketManager> socket_manager{
      new hostapd::SocketManager(5, FLAGS_hostapd_control_dir,
                                 FLAGS_hostapd_client_dir)};
  hostapd::HostapdControlImpl service{std::move(socket_manager)};

  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_hostapd_grpc_addr,
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  auto server = builder.BuildAndStart();
  LOG(INFO) << "Server listening on " << FLAGS_hostapd_grpc_addr;

  prometheus::Exposer exposer{FLAGS_hostapd_metrics_addr};
  LOG(INFO) << "Metrics listening on " << FLAGS_hostapd_metrics_addr;

  auto registry = std::make_shared<prometheus::Registry>();
  exposer.RegisterCollectable(registry);
  auto metrics_collector = std::thread([registry, &service]() {
    hostapd::Metrics metrics(registry);

    std::unique_lock<std::mutex> l(mu);
    const std::chrono::milliseconds scrape_ms{
        FLAGS_hostapd_metrics_scrape_interval_ms};
    while (!is_interrupt) {
      auto status = interrupt.wait_for(l, scrape_ms);
      if (status == std::cv_status::timeout) {
        metrics.Scrape(&service);
      }
    }
  });

  signal(SIGINT, handler);
  signal(SIGTERM, handler);
  auto interrupt_waiter = std::thread([&server]() {
    std::unique_lock<std::mutex> l{mu};
    while (!is_interrupt) interrupt.wait(l);
    server->Shutdown();
  });

  server->Wait();
  interrupt_waiter.join();
  metrics_collector.join();
  LOG(INFO) << "Server shut down";
}
