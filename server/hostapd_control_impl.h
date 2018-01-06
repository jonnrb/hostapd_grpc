#pragma once

#include "proto/api.grpc.pb.h"
#include "server/socket.h"

namespace hostapd {
class HostapdControlImpl final : public HostapdControl::Service {
 public:
  HostapdControlImpl(std::unique_ptr<Socket>&& socket)
      : _socket(std::move(socket)) {}

  grpc::Status Ping(grpc::ServerContext* context, const PingRequest* ping,
                    PongResponse* pong);

  grpc::Status ListClients(grpc::ServerContext* context, const ListRequest* _,
                           ListResponse* list);

 private:
  std::unique_ptr<Socket> _socket;
};
}  // namespace hostapd
