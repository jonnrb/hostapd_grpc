# hostapd_grpc [![Build Status](https://drone.jonnrb.com/api/badges/jon/hostapd_grpc/status.svg?branch=master)](https://drone.jonnrb.com/jon/hostapd_grpc)

Do you use `hostapd` to run your Wi-Fi? Did you know there's a nifty control
socket it exports to let you do normal router stuff with it? Yeah neither did
I. I wanted to hack around on it, but the `wpa_ctrl` interface is a little
klunky so I figured as I use parts of it, they can be wrapped in a tidy GRPC
server with the API spec'd all in protobuf.

(Now with Prometheus metrics!)

### Usage

There needs to be a teensy bit of corrdination with hostapd. The usual
`ctrl_interface` option is `/var/run/hostapd`. hostapd\_grpc needs read access
to this directory and the sockets created there. A part of the `wpa_ctrl`
protocol involves creation of a client socket that hostapd can read. While it
doesn't matter what path hostapd and hostapd\_grpc see the `ctrl_interface`
directory at, the client socket directory path *must* be the same as seen by
hostapd and hostapd\_grpc. Additionally, hostapd\_grpc will think anything that
moves inside the `ctrl_interface` directory is a valid hostapd socket and will
treat it as such. Separate the client and the control directories or prepare
for crash and boom.

By default, gRPC pokes out on port 8080 and Prometheus metrics are on port 9090.

Probably not up to date exerpt of `./hostapd_grpc -help`:

```
  Flags from server/server.cc:
    -hostapd_client_dir (Path to place client sockets in; hostapd must be able
      to read the same directory at this path!) type: string
      default: "/var/run/hostapd_grpc"
    -hostapd_control_dir (Path specified by ctrl_interface hostapd option)
      type: string default: "/var/run/hostapd"
    -hostapd_grpc_addr (Address gRPC will bind) type: string
      default: "0.0.0.0:8080"
    -hostapd_metrics_addr (Address Prometheus metrics will bind) type: string
      default: "0.0.0.0:9090"
    -hostapd_metrics_scrape_interval_ms (How often to scrape metrics from
      hostapd in milliseconds) type: uint64 default: 5000
```

### Docker

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
      - hostapd_control:/var/run/hostapd:rw
      - hostapd_clients:/var/run/hostapd_grpc:ro
    network_mode: host

  hostapd_grpc:
    image: jonnrb/hostapd_grpc
    volumes:
      - hostapd_control:/var/run/hostapd:ro
      - hostapd_clients:/var/run/hostapd_grpc:rw
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
