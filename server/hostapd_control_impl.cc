#include "server/hostapd_control_impl.h"

#include <iostream>
#include <regex>
#include <string>

namespace hostapd {
namespace {
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
}  // namespace

grpc::Status HostapdControlImpl::Ping(grpc::ServerContext* context,
                                      const PingRequest* ping,
                                      PongResponse* pong) {
  std::string response;
  switch (_socket->SendRequest("PING", &response)) {
    case 0:
      break;
    case -1:
      return grpc::Status(grpc::StatusCode::INTERNAL, strerror(errno));
    case -2:
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                          "timeout communicating with hostapd");
  }

  if (strcmp(response.c_str(), "PONG\n") != 0) {
    return grpc::Status(grpc::StatusCode::INTERNAL,
                        "expected \"PONG\"; got \"" + response + "\"");
  }

  return grpc::Status::OK;
}

grpc::Status HostapdControlImpl::ListClients(grpc::ServerContext* context,
                                             const ListRequest* _,
                                             ListResponse* list) {
  std::string tok;

  switch (_socket->SendRequest("STA-FIRST", &tok)) {
    case 0:
      break;
    case -1:
      return grpc::Status(grpc::StatusCode::INTERNAL, strerror(errno));
    case -2:
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                          "timeout communicating with hostapd");
  }

  while (strcmp(tok.c_str(), "FAIL\n") != 0 && strcmp(tok.c_str(), "") != 0) {
    auto* client = list->mutable_client()->Add();
    ParseSta(tok, client);
    const auto& addr = client->addr();
    switch (_socket->SendRequest("STA-NEXT " + addr, &tok)) {
      case 0:
        break;
      case -1:
        return grpc::Status(grpc::StatusCode::INTERNAL, strerror(errno));
      case -2:
        return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                            "timeout communicating with hostapd");
    }
  }

  return grpc::Status::OK;
}
}  // namespace hostapd
