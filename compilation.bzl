copt_flags = ["-O3", "-Wfatal-errors", "--std=c++2a", "-Wall", "-Wextra", "-Wpedantic",]
link_flags = ["-lpthread",]

thread_sanitizer_flags = ["-fsanitize=thread", "-pie", "-fPIE",]
