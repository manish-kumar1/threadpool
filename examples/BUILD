load("//:compilation.bzl", "copt_flags", "link_flags")

cc_binary(
  name = "primes",
  srcs  = ["primes.cc"],
  copts = copt_flags,
  deps = ["//:lib_thp"],
  linkopts = link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "usage",
  srcs  = ["usage.cpp"],
  copts = copt_flags,
  deps = ["//:lib_thp"],
  linkopts = link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "reduce",
  srcs  = ["reduce.cpp"],
  copts = ["-I/usr/include/tbb",] + copt_flags,
  deps = ["//:lib_thp"],
  linkopts = link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "matmul",
  srcs  = ["matmul.cpp"],
  copts = ["-I/usr/include/tbb",] + copt_flags,
  deps = ["//:lib_thp"],
  linkopts = link_flags,
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "word_count",
  srcs  = ["wc.cpp"],
  copts = copt_flags,
  deps = ["//:lib_thp"],
  linkopts = link_flags + ["-lstdc++fs"],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "sort",
  srcs = ["sort.cpp"],
  copts = copt_flags,
  deps = ["//:lib_thp",],
  linkopts = link_flags,
  visibility = ["//visibility:public"],
)
