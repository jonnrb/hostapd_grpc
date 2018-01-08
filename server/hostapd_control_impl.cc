#include "server/hostapd_control_impl.h"

#include <iostream>
#include <regex>
#include <string>

namespace hostapd {
namespace {
constexpr auto* deadline_exceeded_message =
    "deadline exceeded communicating with hostapd socket";

/**
 * break "key=value" pair and fill the correct field in |client|
 */
void ParseStaLine(const std::string& line, Client* client) {
  static const std::regex kv_re{"([^=]+)=(.*)"};
  std::smatch match;

  if (!std::regex_match(line, match, kv_re)) {
    std::cerr << "error matching line: \"" << line << "\"" << std::endl;
    return;
  }

  std::ssub_match key = match[1];
  std::ssub_match value = match[2];

  if (strcmp(key.str().c_str(), "flags") == 0) {
    static const std::regex flags_re{"\\[([^\\]]+)\\]"};

    const auto flags_str = value.str();
    std::sregex_iterator begin{flags_str.begin(), flags_str.end(), flags_re};
    std::sregex_iterator end{};

    auto* flags = client->mutable_flag();
    for (auto it = begin; it != end; ++it) {
      std::smatch flag_match = *it;
      *flags->Add() = flag_match[1];
    }
  } else if (strcmp(key.str().c_str(), "connected_time") == 0) {
    client->set_connected_time(stoul(value.str()));
  } else if (strcmp(key.str().c_str(), "idle_msec") == 0) {
    client->set_idle_msec(stoul(value.str()));
  } else if (strcmp(key.str().c_str(), "rx_packets") == 0) {
    client->set_rx_packets(stoul(value.str()));
  } else if (strcmp(key.str().c_str(), "tx_packets") == 0) {
    client->set_tx_packets(stoul(value.str()));
  } else if (strcmp(key.str().c_str(), "rx_bytes") == 0) {
    client->set_rx_bytes(stoul(value.str()));
  } else if (strcmp(key.str().c_str(), "tx_bytes") == 0) {
    client->set_tx_bytes(stoul(value.str()));
  }

  // TODO: more keys
}

/**
 * turn station info string |tok| into structured |client|
 */
void ParseSta(const std::string& tok, Client* client) {
  auto line_begin = tok.begin();
  decltype(line_begin) line_end;
  bool found_lf = false;
  bool have_addr = false;

  for (auto it = tok.begin(); it != tok.end(); ++it) {
    if (found_lf) {
      line_begin = it;
      found_lf = false;
    }

    if (*it == '\n') {
      found_lf = true;
      line_end = it;

      std::string line(line_begin, line_end);

      if (!have_addr) {
        *client->mutable_addr() = line;
        have_addr = true;
      } else {
        ParseStaLine(line, client);
      }
    }
  }
}

template <typename Iterable>
void PingEach(SocketManager* socket_manager, Iterable&& socket_names,
              PongResponse* pong_response) {
  for (auto&& socket_name : socket_names) {
    auto socket = socket_manager->Get(socket_name);

    auto* pong = pong_response->mutable_pong()->Add();
    pong->set_socket_name(socket_name);

    std::string response;
    decltype(pong->mutable_error()) err = nullptr;
    switch (socket->SendRequest("PING", &response)) {
      case 0:
        break;
      case -1:
        err = pong->mutable_error();
        err->set_msg(strerror(errno));
        err->set_code(ErrorCode::INTERNAL);
        err->set_c_errno(errno);
        continue;
      case -2:
        err = pong->mutable_error();
        err->set_msg(deadline_exceeded_message);
        err->set_code(ErrorCode::DEADLINE_EXCEEDED);
        continue;
    }

    if (strcmp(response.c_str(), "PONG\n") != 0) {
      auto* err = pong->mutable_error();
      err->set_msg("expected PONG but got \"" + response + "\"");
      err->set_code(ErrorCode::INTERNAL);
    }
  }
}

void ListClientsEach(ControlSocket* socket, ListClientsResponse* list) {
  std::string tok;

  decltype(list->mutable_error()->Add()) err = nullptr;
  switch (socket->SendRequest("STA-FIRST", &tok)) {
    case 0:
      break;
    case -1:
      err = list->mutable_error()->Add();
      err->set_msg(strerror(errno));
      err->set_code(ErrorCode::INTERNAL);
      err->set_c_errno(errno);
      return;
    case -2:
      err = list->mutable_error()->Add();
      err->set_msg(deadline_exceeded_message);
      err->set_code(ErrorCode::DEADLINE_EXCEEDED);
      return;
  }

  while (strcmp(tok.c_str(), "FAIL\n") != 0 && strcmp(tok.c_str(), "") != 0) {
    auto* client = list->mutable_client()->Add();
    ParseSta(tok, client);
    const auto& addr = client->addr();
    decltype(list->mutable_error()->Add()) err = nullptr;
    switch (socket->SendRequest("STA-NEXT " + addr, &tok)) {
      case 0:
        break;
      case -1:
        err = list->mutable_error()->Add();
        err->set_msg(strerror(errno));
        err->set_code(ErrorCode::INTERNAL);
        err->set_c_errno(errno);
        return;
      case -2:
        err = list->mutable_error()->Add();
        err->set_msg(deadline_exceeded_message);
        err->set_code(ErrorCode::DEADLINE_EXCEEDED);
        return;
    }
  }
}
}  // namespace

grpc::Status HostapdControlImpl::ListSockets(grpc::ServerContext* context,
                                             const ListSocketsRequest* _,
                                             SocketList* list) {
  auto* out_sockets = list->mutable_socket();
  for (auto& socket_name : _socket_manager->Available()) {
    out_sockets->Add()->set_name(std::move(socket_name));
  }

  return grpc::Status::OK;
}

grpc::Status HostapdControlImpl::Ping(grpc::ServerContext* context,
                                      const PingRequest* ping_request,
                                      PongResponse* pong) {
  if (ping_request->socket_name().empty()) {
    PingEach(_socket_manager.get(), _socket_manager->Available(), pong);
  } else {
    PingEach(_socket_manager.get(), ping_request->socket_name(), pong);
  }
  return grpc::Status::OK;
}

grpc::Status HostapdControlImpl::ListClients(grpc::ServerContext* context,
                                             const ListClientsRequest* req,
                                             ListClientsResponse* list) {
  if (req->socket_name().empty()) {
    for (auto& socket_name : _socket_manager->Available()) {
      auto socket = _socket_manager->Get(socket_name);
      ListClientsEach(socket.get(), list);
    }
  } else {
    for (auto& socket_name : req->socket_name()) {
      auto socket = _socket_manager->Get(socket_name);
      ListClientsEach(socket.get(), list);
    }
  }
  return grpc::Status::OK;
}
}  // namespace hostapd
