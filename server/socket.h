#pragma once

#include <mutex>
#include <string>

#include "wpa_ctrl/wpa_ctrl.h"

namespace hostapd {
class Socket {
 public:
  explicit Socket(const std::string& device, const std::string& client_dir);

  ~Socket();

  int SendRequest(const std::string& request, std::string* reply);

 private:
  std::mutex _mu;
  struct wpa_ctrl* _ctrl;
};
}  // namespace hostapd
