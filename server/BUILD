cc_library(
    name = "hostapd_control_impl",
    srcs = ["hostapd_control_impl.cc"],
    hdrs = ["hostapd_control_impl.h"],
    deps = [
        ":socket",
        "//proto:cc_api",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "metrics",
    srcs = ["metrics.cc"],
    hdrs = ["metrics.h"],
    deps = [
        "//proto:cc_api",
        "@com_google_glog//:glog",
        "@prometheus_cpp//:prometheus_cpp",
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
    deps = [
        ":hostapd_control_impl",
        ":metrics",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_glog//:glog",
    ],
)

cc_binary(
    name = "static",
    srcs = ["server.cc"],
    linkopts = ["-static"],
    deps = [
        ":hostapd_control_impl",
        ":metrics",
    ],
)
