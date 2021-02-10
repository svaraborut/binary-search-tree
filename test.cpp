
#include <iostream>
#include <utility>
#include <iterator>

#include <iomanip>

#include "bst.cpp"

#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstdlib>

#include <random>
#include <limits>

#define __TEST_ASSIGN
#define __TEST_BASIC
#define __TEST_ITER
#define __TEST_STOCHASTIC


#define TEST(name) \
    std::cout << "TEST " << (name) << "\n"; \
    try {

#define END_TEST() \
    std::cout << "+++ ALL OK"; } \
    catch (const std::exception& e) { std::cout << "--- ERROR " << e.what(); }   \
    std::cout << std::endl;

//#define ASSERT(cond, ex) if (!(cond)) throw std::logic_error(ex);
#define ASSERT(cond, ex) \
    if (!(cond)) throw std::logic_error(ex); \
    else std::cout << "-- ok: " << (ex) << std::endl;
#define ASSERT_QUIET(cond, ex) if (!(cond)) throw std::logic_error(ex);


template<typename... Types>
void print(Types const&... args) {
    using expander = int[];
    (void)expander{0, (void(std::cout << args << " "), 0)... };
    std::cout << std::endl;
}

template <typename K, typename V, typename Compare = std::less<K>>
struct bstHelpers {

    // Definitions
    using map = bst<K, V, Compare>;
    using value = typename bst<K, V, Compare>::value_type;
    using vector = typename std::vector<std::pair<K, V>>;

    static vector map_to_vector(map& m) {
        return vector{m.begin(), m.end()};
    }

    static map vector_to_map(vector& v) {
        return map{v.begin(), v.end()};
    }

    static void print_vector(const vector& v) {
        for (auto&& p : v) {
            std::cout << "(" << p.first << ":" << p.second << ") ";
        }
        std::cout << std::endl;
    }

    static bool are_vectors_key_eq(const vector& a, const vector& b) {
        Compare c;
        if (a.size() != b.size()) return false;
        for (typename vector::size_type i = 0; i < a.size(); i++) {
            if (c(a[i].first, b[i].first) || c(b[i].first, a[i].first)) return false;
        }
        return true;
    }

    static bool are_vectors_value_eq(const vector& a, const vector& b) {
        if (a.size() != b.size()) return false;
        for (typename vector::size_type i = 0; i < a.size(); i++) {
            if (a[i].second != b[i].second) return false;
        }
        return true;
    }

    static bool are_vectors_eq(const vector& a, const vector& b) {
        return are_vectors_key_eq(a, b) && are_vectors_value_eq(a, b);
    }


    template<typename T>
    static void sort_vector(std::vector<T>& v) {
        Compare c;
        std::sort(v.begin(), v.end(), [&]( const T& lhs, const T& rhs ){
            return c(lhs.first, rhs.first);
        });
    }

    template<typename T>
    static T random_ex_vector(std::vector<T>& v) {
        unsigned int x = std::rand() % v.size();
        T val = v[x];
        v.erase(v.begin() + x);
        return val;
    }

    static bool is_vector_key_ordered(const vector& v) {
        Compare c;
        K prev;
        bool first = true;
        for (auto&& p : v) {
            if (first) {
                first = false;
                prev = p.first;
            } else {
                // Prev key is not smaller than current
                if (!c(prev, p.first)) {
                    return false;
                }
            }
        }
        return true;
    }

    static bool is_vector_key_greater(K lower, const vector& v) {
        Compare c;
        for (auto&& p : v)
            if (c(p.first, lower)) return false;
        return true;
    }

    static bool is_vector_key_lower(K upper, const vector& v) {
        Compare c;
        for (auto&& p : v)
            if (c(upper, p.first)) return false;
        return true;
    }

    static bool lower(K lower, K upper) {
        Compare c;
        return c(lower, upper);
    }

};

// DUMMY MAPS

bst<int, std::string> dummy_is_map(std::size_t size = 10) {
    using map = bst<int, std::string>;
    map t{};
    for (auto i = 1ul; i <= size; i++)
        t.insert(map::value_type{i, map::value_type::second_type{}});
    return t;
}

using int_pair = std::pair<const int, int>;
static const std::vector<int_pair> random_unique_array(std::size_t many = 10) {
    // Compex random
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution{
        std::numeric_limits<int>::min(),
        std::numeric_limits<int>::max()
    };

    std::vector<int_pair> buff{};
    for (std::size_t i = 0; i < many; i++) {
        // Generate unique list
        while(true) {
            int rnd = distribution(generator);
            bool ok = true;
            for (std::size_t j = 0; j < i; j++)
                if (buff[j].first == rnd)
                    ok = false;
            if (ok) {
                buff.emplace_back(int_pair{rnd, distribution(generator)});
                break;
            }
        }
    }
    return buff;
}

bst<int, int> dummy_ii_map(const std::vector<int_pair>& v) {
    bst<int, int> t{};
    for (auto&& i : v) t.insert(i);
    return t;
}

bst<int, int> dummy_ii_map(const std::vector<int>& v) {
    bst<int, int> t{};
    for (auto&& i : v) t.insert(int_pair{i, (0x23223 + i) % 0x56});
    return t;
}

bst<int, int> dummy_ii_map(std::size_t size = 10) {
    return dummy_ii_map(random_unique_array(size));
}

template<typename Iter>
unsigned int count_iter(Iter begin, Iter end) {
    auto c = 0ul;
    while(begin != end) { c++; ++begin; }
    return c;
}

int main() {
    std::cout << "Testing bst<K, V>" << std::endl;

    using K = int;

    // TEST

#ifdef __TEST_ASSIGN
    TEST("Assignment / Constructor")
    {
        using V = std::string;
        using pair = std::pair<K, V>;
        using map = bst<K, V>;
        using helper = bstHelpers<K, V>;

        // Create
        map map1 = dummy_is_map(5);
        ASSERT(map1.size() == 5, "Map size should be 5");

        // Override
        map1 = map{};
        ASSERT(map1.size() == 0, "Map size should be 0 after override");

        // Copy
        map map2 = map1;
        map2[1] = "";
        map2[2] = "";
        map1[1] = "";

        ASSERT(map2.size() == 2, "map2 size should be 2");
        ASSERT(map1.size() == 1, "map1 size should be 1");

        // Copy constructor
        map map2_1{map2};
//        std::cout << "map2_1 "; map2_1.info_tree();
//        std::cout << "map2 "; map2.info_tree();
        ASSERT(map2_1 != map2, "map2_1 copy of map2 should be is different");
        ASSERT(!(map2_1 == map2), "map2_1 copy of map2 should be not equal");

        // Move constructor
        map map2_2{std::move(map2)};
//        std::cout << "map2_2 "; map2_2.info_tree();
//        std::cout << "map2 "; map2.info_tree();
        ASSERT(map2_2 != map2, "map2_2 move of map2 should be different");
        ASSERT(map2.size() == 0, "map2 should be empty");

        // Copy assignment
        map map3 = map1;
//        std::cout << "map1 "; map1.info_tree();
//        std::cout << "map3 "; map3.info_tree();
        ASSERT(map2_1 != map2, "map3 (copy of map1) should be different");
        ASSERT(!(map2_1 == map2), "map3 (copy of map1) should be not equal");

        map3 = std::move(map1);
//        std::cout << "map3 "; map3.info_tree();
//        std::cout << "map1 "; map1.info_tree();
        ASSERT(map3.size() == 1, "map3 (move of map1) should have size 1");
        ASSERT(map1.size() == 0, "map1 (moved away) should have size 0");

        // Iterator constructor
        helper::vector v{pair{1, "a"}, pair{2, "b"}, pair{3, "c"}};
        map map4{v.begin(), v.end()};
        ASSERT(map4.size() == 3, "map4 should have 3 elements");
        ASSERT(helper::are_vectors_eq(v, helper::map_to_vector(map4)), "map4 should be equal to source vector");

    }
    END_TEST()
#endif

#ifdef __TEST_BASIC
    TEST("Basic")
    {
        using V = int;
        using pair = std::pair<K, V>;
        using map = bst<K, V>;
        using helper = bstHelpers<K, V>;
        using helper = bstHelpers<K, V>;

        map m = map{};

        // Map is empty
        ASSERT(m.size() == 0, "Empty map should be empty");
        ASSERT(m.empty(), "Empty map should be empty");
        ASSERT(helper::map_to_vector(m).empty(), "Empty map should have no content");

        // Insert
        auto _1 = pair{99, 10000};
        std::pair<map::iterator, bool> r = m.insert(_1);
        ASSERT(r.second, "New insert should return true");
        ASSERT(m.size() == 1, "After first insert() size should be 1");
        ASSERT(m[_1.first] == _1.second, "Inserted value is not correct");

        // Find
        map::iterator it = m.find(99);
        ASSERT(it != m.end(), "Key should be present");
        ASSERT(it->first == 99, "Iterator should be on found key");

        // Has
        ASSERT(m.has(99), "Should have this key");
        ASSERT(!m.has(999), "This key should not be present");

        // Insert []
        m[1] = 123456;
        ASSERT(m[1] == 123456, "operator[] should insert value");

        // Update
        r = m.insert(pair{1, 111});
        ASSERT(!r.second, "duplicate insert() should fail");
        ASSERT(m[1] == 123456, "duplicate insert() should not override value");

        // Update []
        m[1] = 111;
        ASSERT(m[1] == 111, "duplicate [] access should update value");

        // Emplace
        m = map{};
        m.emplace(pair{1, 1}, pair{2, 2}, pair{2, 3});
        ASSERT(m.size() == 2, "After emplace() size should be 2");
        ASSERT(m[1] == 1, "Emplace should insert key[1]");
        ASSERT(m[2] != 3, "Emplace should not override keys");
        ASSERT(m[2] == 2, "Emplace should insert key[2]");

        // Keys are sorted
        auto v = helper::map_to_vector(m);
//        helper::print_vector(v);
        ASSERT(helper::is_vector_key_ordered(v), "Keys should be sorted");

        // Erase
        m = map{};
        m[1] = 123;
        m[2] = 321;
        m.erase(2);
        ASSERT(m[1] == 123, "m[1] should not be erased");
        ASSERT(!m.has(2), "m[2] should be gone");

        // Clear
        m.clear();
//        m.info_tree();
        ASSERT(m.size() == 0, "Size should be 0 after clear");
        ASSERT(m.depth() == 0, "Depth should be 0 after clear");

    }
    END_TEST()
#endif

#ifdef __TEST_ITER
    TEST("Iterable")
    {
        using V = std::string;
//        using pair = std::pair<K, V>;
        using map = bst<K, V>;
//        using helper = bstHelpers<K, V>;

        map n = map{};

        // Dummy checks
        auto begin = n.begin();
        ASSERT(begin == begin, "begin() should equal to itself");
        auto end = n.end();
        ASSERT(end == end, "end() should equal to itself");
        ASSERT(n.begin() == n.end(), "begin() == end() on an empty map");
        ASSERT(!(n.begin() != n.end()), "!(begin() != end()) on an empty map");

        // Insert iterator
        std::pair<map::iterator, bool> ip = n.insert(map::value_type{1, "some"});
        ASSERT(ip.second == true, "insert().second should be true");
        ASSERT(ip.first->first == 1, "insert() iterator should point to inserted key");
        ASSERT(ip.first->second == "some", "insert() iterator should point to inserted value");
        ++ip.first;
        ASSERT(ip.first == n.end(), "insert() iterator should exhaust on a 1 elem map");

        // Dummy map
        map m = dummy_is_map(10);
        ASSERT(m.begin() != m.end(), "begin() != end() on a non empty map");

        // Other accesses
        auto it = m.begin();

        auto x = (*it).second;
        auto y = it->second;

        // it->first = 1; // can not
        it->second = " ";
        ASSERT(m[it->first] == " ", "Value should be updatable through iterator");

//        std::next(it);
//        std::next(it, 1);

        K _1 = it->first;
        ++it;
        K _2 = it->first;
        --it;
        K _3 = it->first;

//        map::iterator _i = n.end();

        ASSERT(_1 != _2, "After iterator++ key should be different");
        ASSERT(_2 != _3, "After iterator-- key should be different");
        ASSERT(_3 == _1, "After (iterator++)-- key should be the same");

        // Eventually should reach end
        map::iterator _last;
        while((++it) != m.end()) {
            _last = it;
        }

        // At the end
        map::iterator _end = m.end();
        --_end;
        ASSERT(_last == _end, "--end() should be the last element");
        ASSERT(_end->first == 10, "--end() should be the last element (key==10)");

        // Inverted key range
        map::iterator _range = n(1, 0);
        ASSERT(_range == n.end(), "Range should be empty")

        // Range with 1 exact key
        n[5] = "x";
        _range = n(1, 1);
        ASSERT(count_iter(_range, n.end()) == 1, "Range should have 1 element");

        // Range in gap
        _range = n(2, 4);
        ASSERT(count_iter(_range, n.end()) == 0, "Range in gap should be empty");

        const auto _l = 4l, _u = 7l;
        _range = n(_l, _u);
        bool _cond = true;
        while((++_range) != m.end()) {
            print(_l, _range->first, _u);
            _cond = _cond && (_range->first >= _l && _range->first <= _u);
        }
        ASSERT(_cond, "range should return keys >= lower and <= upper");

    }
    END_TEST()
#endif

#ifdef __TEST_STOCHASTIC
    TEST("Stochastic test")
    {
        using V = int;
//        using pair = std::pair<K, V>;
        using map = bst<K, V>;
        using helper = bstHelpers<K, V>;
        using vector = helper::vector;

        map m = dummy_ii_map(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
        ASSERT(m.depth() == 4, "10 elements should distribute on a depth 4");
//        m.print_tree();

        m = dummy_ii_map(std::vector<int>{10, 9, 8, 7, 6, 5, 4, 3, 2, 1});
        ASSERT(m.depth() == 4, "10 elements should distribute on a depth 4");
//        m.print_tree();

        const std::size_t MAP_SIZE = 10000;
        const std::size_t DEPTH_CHECK = 20; // not checking ~10 is optimum, should be around 11
        const std::size_t ERASE_ATTEMPTS = 250;
        const std::size_t INSERT_ATTEMPTS = 250;

        const unsigned int seeds[] = {
            0x123456ul,
            0x654321ul,
            0x111111ul,
            0x789456ul,
            0x946378ul,
            0x735423ul,
            0x123465ul,
        };

        for (unsigned int SD : seeds) {
            // Populate map
            // Seed to be deterministic
            std::srand(SD);
            const auto v = random_unique_array(MAP_SIZE);
            map x = dummy_ii_map(v);
//            print("MAP DEPTH= ", x.depth());
            ASSERT_QUIET(x.size() == MAP_SIZE, "Size should be wrong map size");
            ASSERT_QUIET(x.depth() <= DEPTH_CHECK, "Elements should distribute on a max depth");
//        x.print_tree();

            using v_pair = std::pair<int, int>;
            using v_vector = std::vector<std::pair<int, int>>;

            // V is not sorted
            v_vector srt{v.begin(), v.end()};
            helper::sort_vector(srt);

            // Keys should be ordered
            auto o = helper::map_to_vector(x);
//        x.print_tree();
            ASSERT_QUIET(helper::is_vector_key_ordered(o), "Keys should be sorted");
            ASSERT_QUIET(helper::are_vectors_eq(srt, o), "Vectors should be equal");

            // Removing a key, removes it from the map
            v_vector removed{};

//            print("@@ ", x.size());

            for (unsigned int i = 0; i < ERASE_ATTEMPTS; i++) {
                v_pair el = helper::random_ex_vector(srt);
                removed.emplace_back(el);

                if (i % 2 == 0) {
                    v_pair r = x.pop(el.first);
                    ASSERT_QUIET(r.first == el.first, "pop() ~ Should return right key");
                    ASSERT_QUIET(r.second == el.second, "pop() ~ Should return value");
                } else {
                    x.erase(el.first);
                }

                auto o = helper::map_to_vector(x);

//            if (!helper::are_vectors_eq(srt, o)) {
//                x.print_tree();
//                print("@@ ", x.size(), srt.size(), o.size(), i, el.first);
//                helper::print_vector(srt);
//                helper::print_vector(o);
//            }

                ASSERT_QUIET(helper::are_vectors_eq(srt, o), "erase() ~ Vectors should be equal");
                ASSERT_QUIET(helper::is_vector_key_ordered(o), "erase() ~ Keys should be sorted");
            }
//            ASSERT(true, "erase() shall work");

            // Adding a key, ads it to the map
            for (unsigned int i = 0; i < (MIN(removed.size(), INSERT_ATTEMPTS)); i++) {
                x.insert(removed[i]);
                srt.emplace_back(removed[i]);
                helper::sort_vector(srt);
                auto o = helper::map_to_vector(x);
                ASSERT_QUIET(helper::are_vectors_eq(srt, o), "insert() ~ Vectors should be equal");
                ASSERT_QUIET(helper::is_vector_key_ordered(o), "insert() ~ Keys should be sorted");
            }
//            ASSERT(true, "insert() shall work");

            // A slice should contain only keys within a range
            v_pair lower = helper::random_ex_vector(srt);
            v_pair upper = helper::random_ex_vector(srt);
//        print(lower.first, upper.first);
            if (helper::lower(upper.first, lower.first)) std::swap(lower, upper);
//        print(lower.first, upper.first);

            vector slc{x(lower.first, upper.first), x.end()};
//        vector slc{x(29900 - 1, upper.first), static_cast<map::range_iterator>(x.end())};
//        helper::print_vector(helper::map_to_vector(x));
//        helper::print_vector(slc);

//            print("SLICE LENGTH= ", slc.size());
            ASSERT_QUIET(helper::is_vector_key_ordered(slc), "Slice should be ordered");
            ASSERT_QUIET(helper::is_vector_key_greater(lower.first, slc), "Slice should be >= than lower");
            ASSERT_QUIET(helper::is_vector_key_lower(upper.first, slc), "Slice should be <= than upper");

        }

        ASSERT(true, "Map should work");

    }
    END_TEST()
#endif

    return 0;
}
