# hostapd_grpc [![Docker Automated Build](https://img.shields.io/docker/automated/jonnrb/hostapd_grpc.svg)](https://hub.docker.com/r/jonnrb/hostapd_grpc/) [![Docker Build Status](https://img.shields.io/docker/build/jonnrb/hostapd_grpc.svg)](https://hub.docker.com/r/jonnrb/hostapd_grpc/builds/)

Do you use `hostapd` to run your Wi-Fi? Did you know there's a nifty control
socket it exports to let you do normal router stuff with it? Yeah neither did
I. I wanted to hack around on it, but the `wpa_ctrl` interface is a little
klunky so I figured as I use parts of it, they can be wrapped in a tidy GRPC
server with the API spec'd all in protobuf.

(Now with Prometheus metrics!)

### Usage

TODO: flags

Since I haven't spun out configuration into flags just yet, the best way to use
this right now is via Docker.

gRPC pokes out on port 8080 and Prometheus metrics are on port 9090. It looks
for hostapd control sockets (the `ctrl_interface` hostapd config option; has to
be set per SSID) in `/hostapd_control` until I make flags. It creates ephemeral
client sockets in `/hostapd_clients` which must be present on hostapd's
filesystem and on hostapd\_grpc's filesystem *at that path*.

Example `docker-compose.yml`:

```yaml
version: '2'

services:
  hostapd:
    image: jonnrb/hostapd
    privileged: true
    volumes:
      - ./hostapd.conf.tmpl:/data/hostapd.conf.tmpl:ro
      - /var/run/docker.sock:/var/run/docker.sock:ro
      - hostapd_control:/hostapd_control:rw
      - hostapd_clients:/hostapd_clients:ro
    network_mode: host

  hostapd_grpc:
    image: jonnrb/hostapd_grpc
    volumes:
      - hostapd_control:/hostapd_control:ro
      - hostapd_clients:/hostapd_clients:rw
    networks:
      - private

  prometheus:
    image: prom/prometheus
    command: --config.file=/prometheus.yml --storage.tsdb.path=/prometheus
    volumes:
      - ./prometheus.yml:/prometheus.yml:ro
      - prometheus:/prometheus:rw
    networks:
      - private

networks:
  private:
    internal: true

volumes:
  prometheus:
  hostapd_control:
    driver_opts:
      type: tmpfs
      device: tmpfs
  hostapd_clients:
    driver_opts:
      type: tmpfs
      device: tmpfs
```

### _Work in Progress..._

If you find this neat and whatnot, I'm open to implementing new features and
fixing bugs. Just add an issue or make a PR!

I'm currently using this to provide data to
[wifi_dash](https://github.com/JonNRb/wifi_dash).
