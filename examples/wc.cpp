#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
#include <unordered_map>
#include <fstream>
#include <set>
#include <execution>
#include <glog/logging.h>

#include "include/threadpool.hpp"
#include "include/partitioner.hpp"
#include "include/clock_util.hpp"

namespace fs = std::filesystem;
namespace rng = std::ranges;

template<std::size_t N>
auto word_map = [](const std::string& file_path) {
    std::unordered_map<std::string, unsigned> wm;
    std::array<char, N> line;

    if (!file_path.empty()) {
        std::ifstream ifs{file_path};
        while(ifs.good()) {
            auto gc = ifs.getline(line.data(), N).gcount();
            if (gc < 2) continue; // empty line
            if (ifs.fail() && (gc == N-1)) {
                std::ostringstream err;
                err << file_path << ": length of line > " << N;
                throw std::logic_error(err.str());
            }

#if 0
            std::string_view l{line.data(), gc - 1};
            // regex doesn't work with string_view out of box
            const std::regex rgx{"\\s+"};
            std::for_each(std::sregex_token_iterator(l.cbegin(), l.cend(), rgx, -1),
                          std::sregex_token_iterator(),
                          [](auto&& w) {
                            wm[w] += 1;
                        });
#endif
            const auto line_end = std::next(line.begin(), gc - 1);
            for(auto line_start = line.begin(); line_start != line_end; ) {
                auto x = std::find_if_not(line_start, line_end, isspace);
                auto y = std::find_if(x, line_end, isspace);
                if (std::distance(x, y) > 0)
                    wm[std::string(x, y)] += 1;

                line_start = y;
            }
        }
    }
    return wm;
};

template<typename Iter, typename Size, typename Comp = std::greater<typename Iter::value_type>>
decltype(auto) most_common(Iter s, Iter e, const Size n, Comp comp = Comp{})
{
  //using value_type = std::decay_t<typename Iter::value_type>;
  //static_assert(std::is_copy_assignable_v<value_type>, "data type is not copy assignable");
  using value_type = std::pair<std::string, unsigned>;

  std::vector<value_type> ans;
  ans.reserve(n);

  std::copy_n(s, n, std::back_inserter(ans));
  std::make_heap(ans.begin(), ans.end(), comp);

  std::for_each(std::next(s, n), e, [&](const auto& p) mutable {
    if (comp(p, ans.front())) {
      std::pop_heap(ans.begin(), ans.end(), comp);
      ans.back() = p;
      std::push_heap(ans.begin(), ans.end(), comp);
    }
  });

  std::sort_heap(ans.begin(), ans.end(), comp);
  return ans;
}

int main(int argc, const char* const argv[])
{
  try {
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);

    if (argc < 4) {
      std::cerr << "usage: word_count /path/to/directory top_n file_extensions...\n"
                << "e.g. word_count /tmp/ 10 .txt .cpp .py" << std::endl;
      return 1;
    }

    thp::util::clock_util<std::chrono::system_clock> cu;
    std::filesystem::path dir_path(argv[1]);
    unsigned topn = 10;
    const std::size_t line_size = 10240u;
    std::set<std::string> ext;

    std::istringstream iss(argv[2]);
    iss >> topn;

    for(auto i = 3; i < argc; ++i) {
      ext.insert(std::string(argv[i]));
    }

    std::vector<fs::directory_entry> entries{fs::recursive_directory_iterator(fs::path(argv[1])),
                                             fs::recursive_directory_iterator()};
    std::vector<std::string> file_paths;

    auto matches = [&] (auto&& entry) {
       return entry.exists() && entry.is_regular_file()
           && (ext.find(entry.path().extension()) != ext.end());
    };
    auto path = [] (auto&& e) { return e.path(); };

    auto paths = entries | std::views::filter(matches) | std::views::transform(path);
    rng::copy(paths, std::back_inserter(file_paths));
    auto table_update = [](auto&& tot, auto&& val) {
                            for(auto&& [k,v] : val)
                                tot[k] += v;
                            return std::move(tot); 
                        };

    auto pair_comp = [](auto&& a, auto&& b) { return a.second > b.second; };

    auto print_result = [&](const std::string& profile,
                            const std::unordered_map<std::string,unsigned>& ans,
                            std::ostream& oss = std::cerr)
    {
        auto top_n = most_common(ans.begin(), ans.end(), topn, pair_comp);
        oss << profile << "(" << file_paths.size() << "): " << cu.get_ms() << " ms" << ", ans = " << ans.size() << std::endl;
        for (auto&& [k, v] : top_n)
            oss << v << " : " << k << std::endl;
    };

    {
      cu.now();
      auto ans = std::transform_reduce(std::execution::seq,
                            file_paths.begin(), file_paths.end(),
                            std::unordered_map<std::string, unsigned>{},
                            table_update,
                            word_map<line_size>);
      cu.now();
      print_result("std", ans);
    }
    {
      thp::threadpool tp;
      cu.now();
      using pf = thp::part_algo<decltype(file_paths.begin())>;
      pf algo(file_paths.begin(), file_paths.end());
      algo.step = 1; //std::distance(file_paths.begin(), file_paths.end())/std::thread::hardware_concurrency();
      auto [f] = tp.transform_reduce(file_paths.begin(), file_paths.end(),
                                 std::unordered_map<std::string, unsigned>{},
                                 table_update,
                                 word_map<line_size>,
                                 thp::partitioner(file_paths.begin(), file_paths.end(), algo)
                                );
      auto ans = f.get();
      cu.now();
      print_result("thp", ans);
    }
  } catch(std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}
