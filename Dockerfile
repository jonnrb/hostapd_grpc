from jonnrb/bazel_cc as build

add . /src
run dbazel build //server:static

from scratch
copy --from=build /src/bazel-bin/server/static /hostapd_grpc

entrypoint ["/hostapd_grpc"]
