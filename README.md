# hostapd_grpc

Do you use `hostapd` to run your Wi-Fi? Did you know there's a nifty control
socket it exports to let you do normal router stuff with it? Yeah neither did
I. I wanted to hack around on it, but the `wpa_ctrl` interface is a little
klunky so I figured as use parts of it, they can be wrapped in a tidy GRPC
server with the API spec'd all in protobuf.

## Building

This project uses Bazel and basically only runs on Linux (it adapts hostapd
after all). If you have that sort of environment already set up,
`bazel run //server` and you're good to go (TODO flags I know). Otherwise,
there's a Dockerfile that will actually build the binary and build _another_
Docker image with just the binary (queue up Dream is Collapsing) and all of this
is accessible through a hastily assembled Makefile.

## _Work in Progress..._
