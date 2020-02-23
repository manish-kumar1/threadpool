load(":compilation.bzl", "copt_flags")
load(":compilation.bzl", "link_flags")

cc_library(
  name = "lib_thp",
  srcs = glob(["src/**/*.cpp", "include/**/*.cpp"]),
  hdrs = glob(["include/**/*.hpp"]),
  copts = ["--std=c++2a", "-Iinclude/", "-Wall",] + copt_flags,
  deps = ["//platform:th_config", "@glog//:glog",],
  linkopts = ["-lpthread",] + link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "threadpool",
  srcs = ["main.cc"],
  copts = ["--std=c++2a", "-Iinclude/", "-Wall",] + copt_flags,
  deps = [":lib_thp", "@glog//:glog", "@yaml-cpp//:yaml-cpp",],
  visibility = ["//visibility:public"],
)
