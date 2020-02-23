workspace(
  name = "threadpool"
)

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
  name = "gtest",
  branch = "master",
  commit = "23b2a3b1cf803999fb38175f6e9e038a4495c8a5",
  remote = "https://github.com/google/googletest.git",
)

http_archive(
    name = "com_github_gflags_gflags",
    strip_prefix = "gflags-2.2.2",
    urls = [
        "https://mirror.bazel.build/github.com/gflags/gflags/archive/v2.2.2.tar.gz",
        "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
    ],
    sha256= "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
)

git_repository(
  name = "glog",
  commit = "96a2f23dca4cc7180821ca5f32e526314395d26a",
  remote = "https://github.com/google/glog.git",
  shallow_since = "1553223106 +0900",
)

git_repository(
  name = "yaml-cpp",
  remote = "https://github.com/jbeder/yaml-cpp.git",
  commit = "b2f89386d8f88655e47c4be0c719073dd6308a21"
)
