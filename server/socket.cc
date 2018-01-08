#include "server/socket.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <system_error>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>

namespace hostapd {
ControlSocket::ControlSocket(const std::string& device,
                             const std::string& client_dir)
    : _mu(), _ctrl(wpa_ctrl_open2(device.c_str(), client_dir.c_str())) {
  if (_ctrl == NULL) {
    throw std::system_error(errno, std::generic_category(),
                            "could not connect to " + device);
  }
}

ControlSocket::~ControlSocket() { wpa_ctrl_close(_ctrl); }

int ControlSocket::SendRequest(const std::string& request, std::string* reply) {
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

SocketManager::SocketManager(int limit, const std::string& hostapd_dir,
                             const std::string& client_dir)
    : _limit(limit),
      _hostapd_dir(hostapd_dir),
      _client_dir(client_dir),
      _mu(),
      _sockets() {}

std::shared_ptr<ControlSocket> SocketManager::Get(const std::string& name) {
  std::lock_guard<std::mutex> l{_mu};

  auto s = _sockets[name];

  if (!s) {
    // Evict with no real priority
    if ((signed)_sockets.size() >= _limit) {
      _sockets.erase(_sockets.begin());
    }

    s = _sockets[name];
    s.reset(new ControlSocket(_hostapd_dir + "/" + name, _client_dir));
  }

  return s;
}

std::vector<std::string> SocketManager::Available() const {
  DIR* dir;
  struct dirent* ent;

  dir = opendir(_hostapd_dir.c_str());
  if (dir == NULL) {
    throw std::system_error(errno, std::generic_category(),
                            "could not open " + _hostapd_dir + " for reading");
  }

  std::vector<std::string> ret;
  while ((ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    ret.push_back(std::string(ent->d_name));
  }
  closedir(dir);
  return ret;
}
}  // namespace hostapd
