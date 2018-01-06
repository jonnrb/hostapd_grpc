# hostapd_grpc [![Docker Automated Build](https://img.shields.io/docker/automated/jonnrb/hostapd_grpc.svg)](https://hub.docker.com/r/jonnrb/hostapd_grpc/) [![Docker Build Status](https://img.shields.io/docker/build/jonnrb/hostapd_grpc.svg)](https://hub.docker.com/r/jonnrb/hostapd_grpc/)

Do you use `hostapd` to run your Wi-Fi? Did you know there's a nifty control
socket it exports to let you do normal router stuff with it? Yeah neither did
I. I wanted to hack around on it, but the `wpa_ctrl` interface is a little
klunky so I figured as use parts of it, they can be wrapped in a tidy GRPC
server with the API spec'd all in protobuf.

## _Work in Progress..._
