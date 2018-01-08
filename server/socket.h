#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "wpa_ctrl/wpa_ctrl.h"

namespace hostapd {
class ControlSocket {
 public:
  explicit ControlSocket(const std::string& device,
                         const std::string& client_dir);

  ~ControlSocket();

  int SendRequest(const std::string& request, std::string* reply);

 private:
  std::mutex _mu;
  struct wpa_ctrl* _ctrl;
};

/**
 * Manages the hostapd control sockets, and keeps a limited number of open
 * connections.
 */
class SocketManager {
 public:
  explicit SocketManager(int limit, const std::string& hostapd_dir,
                         const std::string& client_dir);

  /**
   * Pass in the name of a member of the control directory and managed pointer
   * is returned.
   */
  std::shared_ptr<ControlSocket> Get(const std::string& name);

  /**
   * Lists names in the control directory.
   */
  std::vector<std::string> Available() const;

 private:
  int _limit;
  std::string _hostapd_dir;
  std::string _client_dir;

  std::mutex _mu;
  std::unordered_map<std::string, std::shared_ptr<ControlSocket>> _sockets;
};
}  // namespace hostapd
