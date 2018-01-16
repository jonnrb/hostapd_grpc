git_repository(
    name = "org_pubref_rules_protobuf",
    remote = "https://github.com/pubref/rules_protobuf",
    tag = "v0.8.1",
)

load("@org_pubref_rules_protobuf//cpp:rules.bzl", "cpp_proto_repositories")

cpp_proto_repositories()

git_repository(
    name = "prometheus_cpp",
    commit = "743722db96465aa867bf569eb455ad82dab9f819",
    remote = "https://github.com/jupp0r/prometheus-cpp.git",
)

load(
    "@prometheus_cpp//:repositories.bzl",
    "load_prometheus_client_model",
    "load_civetweb",
    "load_com_google_googlebenchmark",
)

load_prometheus_client_model()

load_civetweb()

load_com_google_googlebenchmark()

git_repository(
    name = "com_github_gflags_gflags",
    #commit = "77592648e3f3be87d6c7123eb81cbad75f9aef5a",
    commit = "e292e0452fcfd5a8ae055b59052fc041cbab4abf",
    remote = "https://github.com/gflags/gflags.git",
)

git_repository(
    name = "com_google_glog",
    commit = "3106945d8d3322e5cbd5658d482c9ffed2d892c0",
    remote = "https://github.com/google/glog.git",
)
