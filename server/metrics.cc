#include "server/metrics.h"

#include <iomanip>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <prometheus/gauge.h>
#include <prometheus/gauge_builder.h>
#include <prometheus/registry.h>

#include "proto/api.grpc.pb.h"

namespace hostapd {
Metrics::Metrics(std::shared_ptr<prometheus::Registry> registry)
    : _registry(std::move(registry)),
      _num_clients(nullptr),
      _clients_by_socket{} {
  _num_clients = &(prometheus::BuildGauge()
                       .Name("hostapd_connected_clients")
                       .Help("Number of clients connected to hostapd")
                       .Register(*_registry));
}

namespace {
std::vector<std::string> ListSockets(HostapdControl::Service* service) {
  ListSocketsRequest req{};
  SocketList res{};
  std::vector<std::string> ret{};

  auto status = service->ListSockets(nullptr, &req, &res);
  if (!status.ok()) {
    std::cerr << "error collecting metrics (listing hostapd sockets): "
              << std::quoted(status.error_message().c_str()) << std::endl;
    return ret;
  }

  ret.reserve(res.socket().size());
  for (const auto& socket : res.socket()) {
    ret.push_back(socket.name());
  }
  return ret;
}

std::map<std::string, int> ListClients(const std::vector<std::string> sockets,
                                       HostapdControl::Service* service) {
  ListClientsRequest req{};
  ListClientsResponse res{};

  for (const auto& socket : sockets) {
    *req.mutable_socket_name()->Add() = socket;
  }

  service->ListClients(nullptr, &req, &res);

  std::map<std::string, int> ret{};
  for (const auto& error : res.error()) {
    std::cerr << "error listing clients on socket (code "
              << ErrorCode_Name(error.code())
              << "): " << std::quoted(error.msg().c_str()) << std::endl;
  }
  for (const auto& client : res.client()) {
    ++ret[client.socket_name()];
  }
  return ret;
}
}  // namespace

void Metrics::Scrape(HostapdControl::Service* service) {
  auto sockets = ListSockets(service);
  if (sockets.empty()) {
    for (auto& existing_metric : _clients_by_socket) {
      existing_metric.second->Set(0.0);
    }
    return;
  }
  for (const auto& socket : sockets) {
    auto* metric = _clients_by_socket[socket];
    // new socket
    if (metric == nullptr) {
      std::map<std::string, std::string> labels{{"socket", socket}};
      metric = _clients_by_socket[socket] = &(_num_clients->Add(labels));
    }
  }

  auto clients = ListClients(sockets, service);
  for (auto& metric : _clients_by_socket) {
    metric.second->Set(static_cast<double>(clients[metric.first]));
  }
}
}  // namespace hostapd
