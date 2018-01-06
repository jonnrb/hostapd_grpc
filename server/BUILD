load("@io_bazel_rules_docker//container:image.bzl", "container_image")
load("@io_bazel_rules_docker//container:push.bzl", "container_push")

cc_library(
    name = "hostapd_control_impl",
    srcs = ["hostapd_control_impl.cc"],
    hdrs = ["hostapd_control_impl.h"],
    deps = [
        ":socket",
        "//proto:cc_api",
    ],
)

cc_library(
    name = "socket",
    srcs = ["socket.cc"],
    hdrs = ["socket.h"],
    deps = ["//wpa_ctrl"],
)

cc_binary(
    name = "server",
    srcs = ["server.cc"],
    deps = [":hostapd_control_impl"],
)

cc_binary(
    name = "static",
    srcs = ["server.cc"],
    linkopts = ["-static"],
    deps = [":hostapd_control_impl"],
)

container_image(
    name = "docker",
    entrypoint = ["/static"],
    files = [":static"],
    ports = ["8080"],
    repository = "jonnrb/hostapd_grpc",
)

container_push(
    name = "docker_push",
    format = "Docker",
    image = ":docker",
    registry = "index.docker.io",
    repository = "jonnrb/hostapd_grpc",
    tag = "latest",
)
