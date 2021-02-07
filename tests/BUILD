load("//:compilation.bzl", "copt_flags", "link_flags")

cc_test(
  name = "task_types",
  srcs = ["priority_task_test.cpp"],
  deps = [
        "//:lib_thp",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
  ],
  copts = copt_flags,
  linkopts = link_flags,
  visibility = ["//visibility:__subpackages__"], 
)

cc_test(
  name = "register_types",
  srcs = ["type_register_test.cpp"],
  deps = [
        "//:lib_thp",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
  ],
  copts = copt_flags,
  linkopts = link_flags,
  visibility = ["//visibility:__subpackages__"],
)
