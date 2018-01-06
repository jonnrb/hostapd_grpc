#include "server/socket.h"

#include <array>
#include <cerrno>
#include <mutex>
#include <system_error>

namespace hostapd {
Socket::Socket(const std::string& device, const std::string& client_dir)
    : _mu(), _ctrl(wpa_ctrl_open2(device.c_str(), client_dir.c_str())) {
  if (_ctrl == nullptr) {
    throw std::system_error(errno, std::generic_category(),
                            "could not connect to |path|");
  }
}

Socket::~Socket() { wpa_ctrl_close(_ctrl); }

int Socket::SendRequest(const std::string& request, std::string* reply) {
  constexpr auto buf_size = 4096;
  std::array<char, buf_size> buf;

  size_t reply_size = buf_size - 1;
  int ret;
  {
    std::lock_guard<std::mutex> l(_mu);
    ret = wpa_ctrl_request(_ctrl, request.c_str(), request.size(), buf.data(),
                           &reply_size, nullptr);
  }
  if (ret != 0) return ret;

  buf[reply_size] = '\0';
  reply->assign(buf.data(), reply_size + 1);

  return 0;
}
}  // namespace hostapd
