load(":compilation.bzl", "copt_flags", "link_flags")
load("//bazel:file-count.bzl", 'file_count_rule')

cc_library(
  name = "lib_thp",
  srcs = glob(["src/**/*.cpp", "include/**/*.cpp"]),
  hdrs = glob(["include/**/*.hpp"]),
  copts = copt_flags + ["-Iinclude/"],
  deps = ["//platform:th_config", "@glog//:glog",],
  linkopts = link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "threadpool",
  srcs = ["main.cc"],
  copts = copt_flags,
  deps = [":lib_thp", "@glog//:glog", "@yaml-cpp//:yaml-cpp",],
  visibility = ["//visibility:public"],
)

file_count_rule(
  name = 'file_count',
  deps = ['threadpool'],
  extension = 'h',
)
