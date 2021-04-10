# Workflow
A high throughput threadpool implementation with configurable job queue, scheduling policies, and data partitioning options.

# Basics
  Consists of job queues with configurable worker threads. Worker threads can be configured with platform specific parameters, which are stored as jobqueue configuration parameters.
  Based on producer-consumer model, worker threads execute tasks in queues as arranged by configured scheduling algorithm. To increase efficiency, worker threads could be bound to specific cpu group(platform dependent)
  Tasks are spread across task queues based on task's type, e.g. simple_task, priority_task.
## Scheduling algorithm

  Threadpool has a helper scheduler class, which can be configured with specific/configurable scheduling algorithm when created.

# Task type 
  There are two class of tasks, simple_task and priority_task. Priority tasks are arranged by its priority and executed accordingly,
  subjected to scheduling delays of the platform.
 ```
  e.g. simple_task<ReturnType>
       simple_task<ReturnType, PriorityType>
```
  There are convenient APIs to create task, e.g.
```
  auto p0 = thp::simple_task<int>(print_prime, 10);
  auto p1 = thp::simple_task<int, float>(print_prime, 42); p1.set_priority(4.2f);
```

# Build
  Threadpool uses bazel to build workspace and maintain external depenencies, e.g. gtest, glog.
  Additionally, to see benchmarks in examples directory, stl libraries with parallel execution support needs to
  be installed on the system. For GCC >= 9.1, stl parallel algorithms have support for intel tbb library on Linux.

```
  git clone https://github.com/manish-kumar1/threadpool.git
  cd threadpool
  bazel build examples:all
  bazel run examples:usage
  bazel run examples:reduce
```
  
# example Usage
  simple usage example:<br>
```
  threadpool tp;
  // simple task<ReturnType>
  auto p0 = thp::simple_task<void>([] { std::cerr << "Hello world" << std::endl; });
  // task with priority setup
  auto p1 = thp::priority_task<unsigned, float>(factorial, 3); p1.set_priority(-30.5f);

  std::vector<decltype(p1)> tasks;
  tasks.emplace_back(thp::simple_task<unsigned, float>(factorial, 4));
  tasks.emplace_back(thp::simple_task<unsigned, float>(factorial, 5));

  auto [fu0, fut1] = tp.schedule(p0, p1);
  auto futs = tp.schedule(tasks);

  tp.drain();
  tp.shutdown();

```
  There are multiple examples of usage and benchmark in examples directory for reference.

