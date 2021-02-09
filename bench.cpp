#include <iostream>
#include <iomanip>

#include <chrono>
#include <map>
#include <cstdlib>
#include "bst.cpp"

struct stats {

    std::string name;
    std::size_t actions;
    std::size_t positive{0};
    std::size_t negative{0};
    float elapsed{0};
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;

    stats(std::string name, std::size_t actions):
        name{name},
        actions{actions},
        begin{std::chrono::steady_clock::now()}
    { }

    void done(bool print = false) {
        end = std::chrono::steady_clock::now();
        elapsed = ((float)std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count())
                / 1000000.0;
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

template<class... Types>
void print_table(Types&&... args) {
    const auto WIDTH = 16ul;
    const auto COLS = { "Action", "Took", "total", "pos_rate", "neg_rate", "positive", "negative" };

    // print: ----------------
    auto line = [WIDTH](auto ROW) {
        for (auto&& i : ROW) std::cout << std::setw(WIDTH) << std::setfill('-') << "";
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


int main() {

    std::cout << "Starting:" << std::endl;

//    const std::size_t INSERT = 100;
//    const std::size_t FIND = 100;
//    const std::size_t REMOVES = 100;

//    const std::size_t INSERT = 10000;
//    const std::size_t FIND = 10000;
//    const std::size_t REMOVES = 10000;

    const std::size_t INSERT = 5000000;
    const std::size_t FIND = 5000000;
    const std::size_t REMOVES = 5000000;

//    const auto SEED = 0x654321ul;
    const auto SEED = 0x123456ul;

    using K = int;
    using V = int;
    using pair = std::pair<K, V>;

//    std::cout << "sizeof(int*) " << sizeof(int*) << std::endl;

    // Benchmark std::map
    {
        std::srand(SEED);
        std::map<K, V> _map;

        // Estimate size
        // !! value is not part of the node
        std::cout << "sizeof(map<>::node_type) " << sizeof(std::map<K, V>::node_type) << std::endl;
        std::cout << "sizeof(std::map<K, V>::value_type) " << sizeof(std::map<K, V>::value_type) << std::endl;
        // Hence we assume the structure is a tree, with data present as leafs
        // https://www.quora.com/How-many-nodes-does-a-full-binary-tree-with-N-leaves-contain
        const std::size_t LEAFS = INSERT;
        const std::size_t NODES = 2 * LEAFS - 1;
        const std::size_t BRANCHES = NODES - LEAFS;
        const std::size_t SIZE =
                BRANCHES * sizeof(std::map<K, V>::node_type)
                + LEAFS * sizeof(std::map<K, V>::value_type);
        std::cout << "estimated max size= " << SIZE << std::endl;


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

    // Benchmark bst
    {
        std::srand(SEED);
        bst<K, V> _map;

        // Estimate size
        std::cout << "sizeof(bst<>::node_type) " << sizeof(bst<K, V>::node_type) << std::endl;
        std::cout << "sizeof(bst<K, V>::value_type) " << sizeof(bst<K, V>::value_type) << std::endl;
        // Data is present on nodes
        const long long int NODES = INSERT;
        const long long int SIZE = NODES * sizeof(bst<K, V>::node_type);
        std::cout << "estimated max size= " << SIZE << std::endl;

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

    return 0;
}
