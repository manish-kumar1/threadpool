load("//:compilation.bzl", "copt_flags")
load("//:compilation.bzl", "link_flags")

cc_library(
  name = "th_config",
  srcs = ["thread_config.c"],
  hdrs = ["thread_config.h"],
  visibility = ["//visibility:public"],
  linkopts = ["-lpthread", ] + link_flags,
  copts = copt_flags,
)

