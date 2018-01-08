#include <condition_variable>
#include <csignal>
#include <mutex>
#include <string>
#include <thread>

#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>

#include "server/hostapd_control_impl.h"

namespace {
std::condition_variable interrupt{};
std::mutex mu{};
}  // namespace

void handler(int _) { interrupt.notify_all(); }

int main(int argc, char** argv) {
  const std::string server_address{"0.0.0.0:8080"};
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

  signal(SIGINT, handler);
  auto t = std::thread([&server]() {
    std::unique_lock<std::mutex> l{mu};
    interrupt.wait(l);
    server->Shutdown();
  });

  server->Wait();
  t.join();
  std::cout << "Server shut down" << std::endl;
}
