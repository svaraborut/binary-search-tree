#include <iostream>
#include <iomanip>
#include <fstream>

#include <chrono>
#include <vector>
#include <map>
#include <tuple>
#include <cstdlib>
#include <random>
#include <limits>

#include "bst.cpp"


#define __BENCHMARK_MAP
#define __BENCHMARK_BSD
//#define __PROFILE_MAP
//#define __PROFILE_BSD
//#define __PROFILE_DEPTH

#define INSERT      500000 /* 5000000 */
#define FIND        INSERT
#define REMOVES     INSERT
#define SEED        0x123456ul /* 0x654321ul */

#define PROFILE     50000 /* 5000000 */
#define BATCHES      1000 /* 50000 */

//#define PROFILE     50000000
//#define BATCHES     500000


struct stats {

    std::string name;
    std::size_t actions;
    std::size_t positive{0};
    std::size_t negative{0};
    double elapsed{0};
    std::chrono::high_resolution_clock::time_point begin;
    std::chrono::high_resolution_clock::time_point end;

    stats(std::string&& name, std::size_t actions):
        name{name},
        actions{actions},
        begin{std::chrono::high_resolution_clock::now()}
    { }

    void done(bool print = false) {
        end = std::chrono::high_resolution_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - begin).count();
        std::size_t total = actions;

        if (print) {
            std::cout << name << " took => "
                      << elapsed << "[s] " // "[µs] "
                      << " positive=" << positive
                      << " negative=" << negative
                      << " total=" << total
                      << " pos_rate=" << (total != 0 ? ((float) positive / (float) total) : 0)
                      << " neg_rate=" << (total != 0 ? ((float) negative / (float) total) : 0)
                      << std::endl;
        }
    }

};

template<typename... Ex>
struct timing_bundle {

    using size_t = std::size_t;
    using duration_t = double;
    using point_t = std::tuple<size_t, size_t, duration_t, Ex...>;

    // x = lower, upper
    // t = time, ...
    std::vector<std::string> titles;
    std::vector<point_t> points;
    size_t last_lower{0};
    std::chrono::high_resolution_clock::time_point last_begin;

    explicit timing_bundle(std::vector<std::string>&& titles): titles{titles} {}

    void start(size_t lower) {
        last_lower = lower;
        last_begin = std::chrono::high_resolution_clock::now();
    }

    void end(size_t upper, Ex... args) {
        // Stopwatch
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        duration_t elapsed = std::chrono::duration_cast<std::chrono::duration<duration_t>>(end - last_begin).count();
        // Add point
        points.emplace_back(point_t{last_lower, upper, elapsed, args...});
    }

    template<std::size_t... I>
    void print(std::ofstream& os, std::index_sequence<I...>) {
        // Header
        os << "lower,upper,duration";
        for (auto&& c: titles) os << "," << c;
        os << "\n";

        // Rows
        for (point_t p : points) {
            (..., (os << (I == 0 ? "" : ",") << std::get<I>(p)));
            os << "\n";
        }
    }

    void dump(std::string&& path) {
        std::ofstream file;
        file.open(path);
        print(file, std::make_index_sequence<sizeof...(Ex) + 3>());
        file.close();
    }

};

template<typename... Types>
void print_table(Types&&... args) {
    const auto WIDTH = 16ul;
    const auto COLS = { "Action", "Took", "total", "pos_rate", "neg_rate", "positive", "negative" };

    // print: ----------------
    auto line = [WIDTH](auto ROW) {
        for (auto&& i : ROW) {
            (void)i;
            std::cout << std::setw(WIDTH) << std::setfill('-') << "";
        }
        std::cout << "--" << std::endl;
    };
    // Row
    auto cell = [WIDTH](auto VALUE) {
        std::cout << "| " << std::left << std::setw(WIDTH - 3) << std::setfill(' ') << VALUE << " ";
    };
    auto endl = [WIDTH]() {
        std::cout << " |" << std::endl;
    };
    // print: | value | value | value
    auto row = [WIDTH](auto ROW) {
        for (auto&& i : ROW)
            std::cout << "| " << std::left << std::setw(WIDTH - 3) << std::setfill(' ') << i << " ";
        std::cout << " |" << std::endl;
    };

    line(COLS);
    row(COLS);
    line(COLS);

    for (auto&& a : { args... }) {
        cell(a.name);
        cell(a.elapsed);
        cell(a.actions);
        cell(a.actions != 0 ? ((float) a.positive / (float) a.actions) : 0);
        cell(a.actions != 0 ? ((float) a.negative / (float) a.actions) : 0);
        cell(a.positive);
        cell(a.negative);
        endl();
        line(COLS);
    }
}

template<typename Map>
std::size_t map_size(Map map, std::size_t size_over = 0) {
    // Estimate size
    // !! value is not part of the node
    // Hence we assume the structure is a tree, with data present as leafs
    // https://www.quora.com/How-many-nodes-does-a-full-binary-tree-with-N-leaves-contain
    const std::size_t LEAFS = size_over > 0 ? size_over : map.size();
    const std::size_t NODES = 2 * LEAFS - 1;
    const std::size_t BRANCHES = NODES - LEAFS;
    return BRANCHES * sizeof(typename Map::node_type);
            // + LEAFS * sizeof(Map::value_type);
}

template<typename Bst>
std::size_t bst_size(Bst bst, std::size_t size_over = 0) {
    // Value is contained within node
    return (size_over > 0 ? size_over : bst.size())
        * (sizeof(typename Bst::node_type)- sizeof(typename Bst::value_type));
}


int main() {

    std::cout << "Starting:" << std::endl;

    using K = std::size_t;      // used to benchmark on 64 bits
    using V = int;
    using pair = std::pair<K, V>;

    std::cout << "sizeof(int*) " << sizeof(int*) << std::endl;

    // Benchmark std::map
#ifdef __BENCHMARK_MAP
    {
        std::srand(SEED);
        std::map<K, V> _map;

        // Estimate size
        // !! value is not part of the node
        std::cout << "sizeof(map<>::node_type) " << sizeof(std::map<K, V>::node_type) << std::endl;
        std::cout << "sizeof(std::map<K, V>::value_type) " << sizeof(std::map<K, V>::value_type) << std::endl;
        std::cout << "estimated normalized size= " << map_size(_map, INSERT) << std::endl;


        stats _insert{"map<> Insert", INSERT};
        for (std::size_t i = 0; i < INSERT; i++) {
            _map.insert(pair{std::rand(), 0});
            _insert.positive++;
        }
        _insert.done();

        stats _find{"map<> Find", FIND};
        for (std::size_t i = 0; i < FIND; i++) {
            if (_map.find(std::rand()) != _map.end()) {
                _find.positive++;
            } else {
                _find.negative++;
            }
        }
        _find.done();

        stats _removes{"map<> Erase", REMOVES};
        for (std::size_t j = 0; j < REMOVES; j++) {
            if (_map.erase(std::rand()) > 0) {
                _removes.positive++;
            } else {
                _removes.negative++;
            }
        }
        _removes.done();

        print_table(_insert, _find, _removes);
    }
#endif

#ifdef __BENCHMARK_BSD
    // Benchmark bst
    {
        std::srand(SEED);
        bst<K, V> _map;

        // Estimate size
        std::cout << "sizeof(bst<>::node_type) " << sizeof(bst<K, V>::node_type) << std::endl;
        std::cout << "sizeof(bst<K, V>::value_type) " << sizeof(bst<K, V>::value_type) << std::endl;
        std::cout << "estimated normalized size= " << bst_size(_map, INSERT) << std::endl;

        stats _insert{"bst<> Insert", INSERT};
        for (std::size_t i = 0; i < INSERT; i++) {
            _map.insert(pair{std::rand(), 0});
            _insert.positive++;
        }
        _insert.done();

        stats _find{"bst<> Find", FIND};
        for (std::size_t i = 0; i < FIND; i++) {
            if (_map.find(std::rand()) != _map.end()) {
                _find.positive++;
            } else {
                _find.negative++;
            }
        }
        _find.done();

        stats _removes{"bst<> Erase", REMOVES};
        for (std::size_t j = 0; j < REMOVES; j++) {
            if (_map.erase(std::rand()) > 0) {
                _removes.positive++;
            } else {
                _removes.negative++;
            }
        }
        _removes.done();

        print_table(_insert, _find, _removes);
    }
#endif

#ifdef __PROFILE_MAP
    {
        using rnd_t = unsigned int;
        std::default_random_engine generator;
        std::uniform_int_distribution<rnd_t> distribution(0,std::numeric_limits<rnd_t>::max());

        std::map<K, V> _map;

        enum action{Insert=0, Find=1, Erase=2, Erase_Hit=3};
        timing_bundle<action, std::size_t, std::size_t, std::size_t, std::size_t>
                profiler{{ "action", "depth", "size", "hit", "miss" }};

        // Profile insert
        for (std::size_t j = 0; j < PROFILE; j += BATCHES) {
            rnd_t _start = distribution(generator);
            std::size_t hit{0}, miss{0};
            profiler.start(j + 1);
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.insert(pair{_start + i, 0}).second) hit++;
                else miss++;
            }
            profiler.end(j + BATCHES + 1, Insert, 0, map_size(_map), hit, miss);

            // Profile Erase
            hit = 0; miss = 0;
            profiler.start(_map.size());
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.erase(_start + i) > 0) hit++;
                else miss++;
            }
            profiler.end(_map.size(), Erase_Hit, 0, map_size(_map), hit, miss);

            // Fill back up a batch
            while(_map.size() < (j + BATCHES)) {
                _map.insert(pair{distribution(generator), 0});
            }

            std::cout << "- " << j << std::endl;
        }

        // Batch profilation
        _map.clear();
        for (std::size_t j = 0; j < PROFILE; j += BATCHES) {
            // Fill up a batch
            while(_map.size() < (j + BATCHES)) {
                _map.insert(pair{distribution(generator), 0});
            }

            // Profile Erase
            std::size_t hit{0}, miss{0};
            rnd_t _start = distribution(generator);
            profiler.start(_map.size());
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.erase(_start + i) > 0) hit++;
                else miss++;
            }
            profiler.end(_map.size(), Erase, 0, map_size(_map), hit, miss);

            // Fill up a batch
            while(_map.size() < (j + BATCHES)) {
                _map.insert(pair{distribution(generator), 0});
            }

            // Profile Find
            hit = 0; miss = 0; _start = distribution(generator);
            profiler.start(_map.size());
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.find(_start + i) != _map.end()) hit++;
                else miss++;
            }
            profiler.end(_map.size(), Find, 0, map_size(_map), hit, miss);

            std::cout << "- " << j << std::endl;
        }

        // Dump
        profiler.dump("./map_dump.csv");
    };
#endif

#ifdef __PROFILE_BSD
    {
        using rnd_t = unsigned int;
        std::default_random_engine generator;
        std::uniform_int_distribution<rnd_t> distribution{0,std::numeric_limits<rnd_t>::max()};


        bst<K, V> _map;

        enum action{Insert=0, Find=1, Erase=2, Erase_Hit=3};
        timing_bundle<action, std::size_t, std::size_t, std::size_t, std::size_t>
                profiler{{ "action", "depth", "size", "hit", "miss" }};

        // Profile insert
        for (std::size_t j = 0; j < PROFILE; j += BATCHES) {
            rnd_t _start = distribution(generator);
            std::size_t hit{0}, miss{0};
            profiler.start(j + 1);
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.insert(pair{_start + i, 0}).second) hit++;
                else miss++;
            }
            profiler.end(j + BATCHES + 1, Insert, _map.depth(), bst_size(_map), hit, miss);

            // Profile Erase
            hit = 0; miss = 0;
            profiler.start(_map.size());
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.erase(_start + i) > 0) hit++;
                else miss++;
            }
            profiler.end(_map.size(), Erase_Hit, _map.depth(), bst_size(_map), hit, miss);

            // Fill back up a batch
            while(_map.size() < (j + BATCHES)) {
                _map.insert(pair{distribution(generator), 0});
            }

            std::cout << "- " << j << std::endl;
        }

        // Batch profilation
        _map.clear();
        for (std::size_t j = 0; j < PROFILE; j += BATCHES) {
            // Fill up a batch
            while(_map.size() < (j + BATCHES)) {
                _map.insert(pair{distribution(generator), 0});
            }

            // Profile Erase
            std::size_t hit{0}, miss{0};
            rnd_t _start = distribution(generator);
            profiler.start(_map.size());
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.erase(_start + i) > 0) hit++;
                else miss++;
            }
            profiler.end(_map.size(), Erase, _map.depth(), bst_size(_map), hit, miss);

            // Fill up a batch
            while(_map.size() < (j + BATCHES)) {
                _map.insert(pair{distribution(generator), 0});
            }

            // Profile Find
            hit = 0; miss = 0; _start = distribution(generator);
            profiler.start(_map.size());
            for (std::size_t i = 0; i < BATCHES; i++) {
                if (_map.find(_start + i) != _map.end()) hit++;
                else miss++;
            }
            profiler.end(_map.size(), Find, _map.depth(), bst_size(_map), hit, miss);

            std::cout << "- " << j << std::endl;
        }

        // Dump
        profiler.dump("./bst_dump.csv");
    };
#endif

#ifdef __PROFILE_DEPTH
    {
        using rnd_t = unsigned int;
        std::default_random_engine generator;
        std::uniform_int_distribution<rnd_t> distribution(0,std::numeric_limits<rnd_t>::max());

        enum action{Random=0, Positive=1, Negative=2};
        timing_bundle<action, std::size_t>
                profiler{{ "action", "depth" }};

        std::size_t j{0};
        bst<K, V> _map;

        // Profile depth random
        while(j < PROFILE) {
            double x = j / (double)PROFILE * 5 + 1e-3;
            double _b = (double)BATCHES * x / (x + 1.0);
            const std::size_t LOCAL_BATCH = (std::size_t)_b;

            rnd_t _start = distribution(generator);
            profiler.start(_map.size());
            for (std::size_t i = 0; i < LOCAL_BATCH; i++) {
                _map.insert(pair{_start + i, 0});
            }
            profiler.end(_map.size(), Random, _map.depth());

            j += LOCAL_BATCH;
        }

        // Profile positive
        j = 0;
        _map.clear();
        while(j < PROFILE) {
            double x = j / (double)PROFILE * 5 + 1e-3;
            double _b = (double)BATCHES * x / (x + 1.0);
            const std::size_t LOCAL_BATCH = (std::size_t)_b;

            profiler.start(_map.size());
            for (std::size_t i = 0; i < LOCAL_BATCH; i++) {
                _map.insert(pair{j + i, 0});
            }
            profiler.end(_map.size(), Positive, _map.depth());

            j += LOCAL_BATCH;
        }

        // Profile negative
        j = 0;
        _map.clear();
        while(j < PROFILE) {
            double x = j / (double)PROFILE * 5 + 1e-3;
            double _b = (double)BATCHES * x / (x + 1.0);
            const std::size_t LOCAL_BATCH = (std::size_t)_b;

            profiler.start(_map.size());
            for (std::size_t i = 0; i < LOCAL_BATCH; i++) {
                _map.insert(pair{PROFILE - j - i, 0});
            }
            profiler.end(_map.size(), Negative, _map.depth());

            j += LOCAL_BATCH;
        }

        // Dump
        profiler.dump("./bst_depth.csv");
    };
#endif



    return 0;
}
