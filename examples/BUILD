load("//:compilation.bzl", "copt_flags")
load("//:compilation.bzl", "link_flags")

cc_binary(
  name = "primes",
  srcs  = ["primes.cc"],
  copts = ["--std=c++2a", "-Wall",] + copt_flags,
  deps = ["//:lib_thp", "@glog//:glog",],
  linkopts = ["-lpthread"] + link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "usage",
  srcs  = ["usage.cpp"],
  copts = ["--std=c++2a", "-Wall",] + copt_flags,
  deps = ["//:lib_thp", "@glog//:glog",],
  linkopts = ["-lpthread", ] + link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "reduce",
  srcs  = ["reduce.cpp"],
  copts = ["--std=c++2a", "-Wall", "-I/usr/include/tbb",] + copt_flags,
  deps = ["//:lib_thp", "@glog//:glog",],
  linkopts = ["-lpthread"] + link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "matmul",
  srcs  = ["matmul.cpp"],
  copts = ["--std=c++2a", "-Wall", "-I/usr/include/tbb",] + copt_flags,
  deps = ["//:lib_thp", "@glog//:glog",],
  linkopts = ["-lpthread", ] + link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "word_count",
  srcs  = ["wc.cpp"],
  copts = ["--std=c++2a", "-Wall", ] + copt_flags,
  deps = ["//:lib_thp", "@glog//:glog",],
  linkopts = ["-lpthread", "-lstdc++fs",] + link_flags,
  visibility = ["//visibility:public"],
)
