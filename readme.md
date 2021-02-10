# Advanced Programming Exam 2021

The project consists of an implementation of a binary search tree. In addition
to the tasks provided, a naive and experimental self balancing approach has been
added.

### 😟 Cheats

Erase function signature has been modified to return a the quantity of elements
deleted.
```c++
size_type erase(const key_type& x);
```

## 💹 Benchmarks

Project also includes a complete benchmark suite (see `bench.cpp`) that performs
comparative tests on `bst<>` against `std::map<>`.

_The `bench` executable outputs two files `bst_dump.csv` and `map_dump.csv` that
contains the profile of the benchmark for `bst` and `map` respectively. This files
shall be copied to `./plot` where the `plot.py` script can be run to generate the
charts._


#### 🤔 Comparative tests

Originally i was planning to make just this simple tabular print. Unfortunatelly
the results were not as expected. How is possible that my dummy implementation of
the `bst<>` is outperforming `std::map` by a 5x 🤔. This calls for further
investigation.

```text
sizeof(map<>::node_type) 8
sizeof(std::map<K, V>::value_type) 8
estimated normalized size= 3999992
------------------------------------------------------------------------------------------------------------------
| Action        | Took          | total         | pos_rate      | neg_rate      | positive      | negative       |
------------------------------------------------------------------------------------------------------------------
| map<> Insert  | 0.683061      | 500000        | 1             | 0             | 500000        | 0              |
------------------------------------------------------------------------------------------------------------------
| map<> Find    | 0.506285      | 500000        | 1             | 0             | 500000        | 0              |
------------------------------------------------------------------------------------------------------------------
| map<> Erase   | 0.912927      | 500000        | 0.065536      | 0.934464      | 32768         | 467232         |
------------------------------------------------------------------------------------------------------------------
sizeof(bst<>::node_type) 24
sizeof(bst<K, V>::value_type) 8
estimated normalized size= 8000000
------------------------------------------------------------------------------------------------------------------
| Action        | Took          | total         | pos_rate      | neg_rate      | positive      | negative       |
------------------------------------------------------------------------------------------------------------------
| bst<> Insert  | 0.146531      | 500000        | 1             | 0             | 500000        | 0              |
------------------------------------------------------------------------------------------------------------------
| bst<> Find    | 0.123607      | 500000        | 1             | 0             | 500000        | 0              |
------------------------------------------------------------------------------------------------------------------
| bst<> Erase   | 0.0717442     | 500000        | 0.065536      | 0.934464      | 32768         | 467232         |
------------------------------------------------------------------------------------------------------------------
```

After extending the benchmark suite the following results appear to still confirm
the original findings. Performances are at a slowly decaying 2x at around 1M entities
being present in the maps, but performances are getting closer with 1.5x at 5M
entities.

![alt text](./plot/5M/ops.png)

The size of the `bst<>` implementation is 2x compared to `map<>`. This is mainly due
to `map<>::node` containing only two pointers, whether `bst<>::node` contains three
pointers and the `depth` cache variable.

_The presented size is only the tree structure size hence excluding both the key
and the value_

![alt text](./plot/5M/size.png)

Further investigation even proves a very steady tree depth growth. Where `positive`
and `negative` insertion (keys are inserted in ascending or descending order
respectively) show almost optimal (log2) performances and random insertion has a
steady 113% of log2.

![alt text](./plot/5M/all_depths.png)

#### 🤔 Conclusions

Honestly I was not expecting such performances, having implemented a naive balancing
approach that just popped in my mind when reading
[Red-Black tree on Wikipedia](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree).
After spending more time in investigating the performances than the original
implementation I came to the conclusion that there is something strange going on.
Maybe:

- There may be something obvious that I'm missing in the testing
- Despite the extensive testing there may still be a bug in the implementation.
- In `std::map` branch nodes has only `left` and `right` pointers making my
  implementation with `parent` pointer faster when traversing the tree upward. Or
  having the `depth` cached in nodes makes the balancing evaluations faster.
- I'm using map in such a way that makes it inefficient.
- `std::map` has a huge penalty when allocating the data having separate leaves to
  store data (eg.: for each insertion has to allocate space for data + for 50% of
  the insertions it has to allocate a new branch node)
- `std::map` has some other allocation penalties
- Red-Black trees are slower than the naive implemented approach but are more stable.
- _Unlikely but possible that my implementation of the map is loosing data somewhere_


## 🧪 Test

Project include a complete test suite (see `test.cpp`). Two types of tests are performed:

- integrity test: hand written tests designed to verify logic and functional integrity
  of the class.
- stochastic tests: that performs insertion and extraction of data from the map.
  tests are performed with pseudo random values with many values, to attempt to discover
  errors in the tree structure (aka loosing data and making more evident memory leaks).


## 🔧 API

##### 🙌🏼 Iteration constructor
```c++
template<typename Iter>
bst(Iter begin, Iter end);
```
Create a map from an iterable iterable source of pair<K,V> values.

##### ✔️ Insert
```c++
std::pair<iterator, bool> insert(const pair_type& x);
std::pair<iterator, bool> insert(pair_type&& x);
```
Inserts a pair in the map. If insertion is successful returns an iterator to the pair and true, else the end() iterator and false.

##### ✔️ Emplace
```c++
std::pair<iterator, bool> emplace(Types&&... args);
```
Inserts multiple pairs in the map. If at least one insertion is successful
returns an iterator to the FIRST INSERTED pair and true. If none of the
values has been inserted end() and false are returned.
_Logic here was not defined in the assignment_.

##### ✔️Erase
```c++
size_type erase(const K& k) noexcept;
```
Removes a key from the map.

##### 🙌🏼 Pop
```c++
value_type pop(const K& k) noexcept;
```
Pops a value from the map returning the value.

##### ✔️ Clear
```c++
void clear() noexcept;
```
Removes all the values from the map

##### ✔️ Balance
```c++
void balance() noexcept;
```
Balances the tree

##### 🙌🏼 Has
```c++
bool has(const K& k) noexcept;
```
Weather the map contains a given key

##### ✔️ Find
```c++
iterator find(const K& k) noexcept;
const_iterator find(const K& k) const noexcept;
```
Searches for a key, if the key is present
an iterator that starts at that key is returned
else end() is returned.

##### 🙌🏼 Size
```c++
size_type size() noexcept;
```
Returns the size of the map O(1)

##### 🙌🏼 Empty
```c++
bool empty() noexcept;
```
Check weather the map is empty O(1)

##### 🙌🏼 Depth
```c++
unsigned int depth() noexcept;
```
Returns the current depth of the map O(1)

##### ✔️ Subscripting operator
```c++
V& operator[](const K& k);
V& operator[](K&& k);
```
Allows for easy lookup with the subscript ( @c [] ) operator.  Returns 
data associated with the key specified in subscript.  If the key does
not exist, a pair with that key is created using default values, which
is then returned.

##### 🙌🏼 Function call operator
```c++
iterator operator()(const K& lower, const K& upper) noexcept;
const_iterator operator()(const K& lower, const K& upper) const noexcept;
```
Returns an iterator to a slice of the map. The slice will start
at the first key greater or equal to lower and will end with the
last key lower or equal to upper.

##### ✔️ Begin
```c++
iterator begin() noexcept;
const_iterator begin() const noexcept;
const_iterator cbegin() const noexcept;
```
An iterator of the map elements starting with the lower key

##### ✔️  End
```c++
iterator end() noexcept;
const_iterator end() const noexcept;
const_iterator cend() const noexcept;
```
The end iterator for this map (a simple nullptr reference)

##### ✔️ Put-to operator
```c++
friend std::ostream& operator<<(std::ostream& os, const bst& x);
```
Prints the json-style representation of the map

##### 🙌🏼 Print tree info (DEBUG)
```c++
void tree_info();
void tree_info(std::ostream& os);
```
Prints the tree info. Example output:
```txt
bst{size=123, root=013307F8}
```

##### 🙌🏼 Print tree structure (DEBUG)
```c++
void print_tree();
void print_tree(std::ostream& os);
```
Prints the tree structure. Example output:
```txt
Size: 10
01330AF8 [depth=3, parent=00000000, left=013307F8, right=01330D98] (4:)
|->L 013307F8 [depth=1, parent=01330AF8, left=01330858, right=01330DF8] (2:)
|    |->L 01330858 [depth=0, parent=013307F8, left=00000000, right=00000000] (1:)
|    |    |->L (empty)
|    |    -->R (empty)
|    -->R 01330DF8 [depth=0, parent=013307F8, left=00000000, right=00000000] (3:)
|         |->L (empty)
|         -->R (empty)
-->R 01330D98 [depth=2, parent=01330AF8, left=01330D38, right=01330B58] (8:)
     |->L 01330D38 [depth=1, parent=01330D98, left=013308B8, right=01330CD8] (6:)
     |    |->L 013308B8 [depth=0, parent=01330D38, left=00000000, right=00000000] (5:)
     |    |    |->L (empty)
     |    |    -->R (empty)
     |    -->R 01330CD8 [depth=0, parent=01330D38, left=00000000, right=00000000] (7:)
     |         |->L (empty)
     |         -->R (empty)
     -->R 01330B58 [depth=1, parent=01330D98, left=00000000, right=01330918] (9:)
          |->L (empty)
          -->R 01330918 [depth=0, parent=01330B58, left=00000000, right=00000000] (10:)
               |->L (empty)
               -->R (empty)
```
