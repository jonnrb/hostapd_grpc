#pragma once

#include <prometheus/gauge.h>
#include <prometheus/registry.h>

#include "proto/api.grpc.pb.h"

namespace hostapd {
class Metrics {
 public:
  Metrics(std::shared_ptr<prometheus::Registry> registry);

  void Scrape(HostapdControl::Service* service);

 private:
  std::shared_ptr<prometheus::Registry> _registry;

  // owned by `_registry`
  prometheus::Family<prometheus::Gauge>* _num_clients;

  std::map<std::string, prometheus::Gauge*> _clients_by_socket;
};
}  // namespace hostapd
