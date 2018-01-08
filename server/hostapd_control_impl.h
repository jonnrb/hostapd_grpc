#pragma once

#include "proto/api.grpc.pb.h"
#include "server/socket.h"

namespace hostapd {
class HostapdControlImpl final : public HostapdControl::Service {
 public:
  HostapdControlImpl(std::unique_ptr<SocketManager>&& socket_manager)
      : _socket_manager(std::move(socket_manager)) {}

  grpc::Status ListSockets(grpc::ServerContext* context,
                           const ListSocketsRequest* _, SocketList* list);

  grpc::Status Ping(grpc::ServerContext* context, const PingRequest* ping,
                    PongResponse* pong);

  grpc::Status ListClients(grpc::ServerContext* context,
                           const ListClientsRequest* _,
                           ListClientsResponse* list);

 private:
  std::unique_ptr<SocketManager> _socket_manager;
};
}  // namespace hostapd
