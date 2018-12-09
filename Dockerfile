from golang:1.11.2 as build
add . /src
run cd /src && go get -v ./cmd/hostapd_grpc

from gcr.io/distroless/base
copy --from=build /go/bin/hostapd_grpc /hostapd_grpc
expose 8080 9090
entrypoint ["/hostapd_grpc"]
