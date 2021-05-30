cc_library(
  name = "spdlog",
  srcs = ["build/libspdlog.a"],
  hdrs = glob(["include/**/*.h",]),
  includes = ["spdlog"],
  visibility = ["//visibility:public"],
  linkstatic = True,
  strip_include_prefix = "include",
)
