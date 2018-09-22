from golang:1.11.0 as build
add . /go/src/go.jonnrb.io/hostapd_grpc
run go get -v go.jonnrb.io/hostapd_grpc/cmd/hostapd_grpc

from gcr.io/distroless/base
copy --from=build /go/bin/hostapd_grpc /hostapd_grpc
expose 8080 9090
entrypoint ["/hostapd_grpc"]
