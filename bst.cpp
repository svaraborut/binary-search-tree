#pragma once

#include <iostream>
#include <utility>
#include <sstream>

#define __EXPERIMENTAL_AUTO_BALANCE
#define __ITERATOR_RECOVERABLE
#define __ITERATOR_LOWER_END

//#define __DEBUG_ITERATOR
//#define __DEBUG_NODE_RAII
//#define __DEBUG_BST_RAII
//#define __DEBUG_BALANCE

#define PV(name, node) \
    std::cout << name << " " << node \
    << " [parent=" << node->parent << ", l=" << node->left << ", r=" << node->right << "]" << std::endl

// Helper functions
#define MAX(a, b) (a) < (b) ? (b) : (a)
#define MIN(a, b) (b) < (a) ? (b) : (a)
#define NNL(_1, _2) (_1) != nullptr ? (_1) : (_2)

// Build a conditional block for comparison
#define TRIPLE_COMPARE(cmp, a, b, less, gt, eq) \
    if (cmp(a, b)) { less; }                    \
    else if (cmp(b, a)) { gt; }                 \
    else { eq; }

// No insertion return pair
#define NOINSERT std::pair<iterator, bool>{ iterator{}, false }

// Make the node become the left child of the parent
#define CHILD_LEFT(p, c)    \
    if (p) (p)->left = c;   \
    if (c) (c)->parent = p;

// Make the node become the right child of the parent
#define CHILD_RIGHT(p, c)   \
    if (p) (p)->right = c;  \
    if (c) (c)->parent = p;

// Replace old_child with new_child to the parent
#define CHILD_AS(p, old_child, new_child)  \
    if (p) {                               \
        if ((p)->left == (old_child))      \
            (p)->left = new_child;         \
        else if ((p)->right == (old_child))\
            (p)->right = new_child;        \
    } else {                               \
        root = (new_child);                \
    }                                      \
    if (new_child) (new_child)->parent = (p);

// Get left branch depth for a node
#define DEPTH_LEFT(node) (node)->left ? ((node)->left->depth + 1) : 0

// Get right branch depth for a node
#define DEPTH_RIGHT(node) (node)->right ? ((node)->right->depth + 1) : 0

// Update node depth value
#define REFRESH_DEPTH(node) (node)->depth = MAX(DEPTH_LEFT(node), DEPTH_RIGHT(node))

// Detach node's children
#define DETACH(node) (node)->left = nullptr; (node)->right = nullptr;


template <typename K, typename V>
struct node;

template<typename elem_type, typename VT>
class _iterator;


template <typename K, typename V, typename Compare = std::less<K>, typename size_type = std::size_t>
class bst {

// DEFINITIONS

    Compare compare;

    using node = node<K, V>;
    using pair_type = std::pair<const K, V>;

    node* root{nullptr};
    size_type _size{0};

// INTERNAL

    // INNER
    enum find_method{EXACT, LEFT, RIGHT};

    /**
     * Provided a local root, find a node by key within the downstream tree.
     * If method is different from EXACT, the function will always return
     * a node that will be the LEFT (or RIGHT) closest node to the searched key.
     * @param current   local root to start from
     * @param k         key to search for
     * @param method    EXACT, LEFT, RIGHT
     * @return          the found node or nullptr
     */
    node* __find_key(node* current, const K& k, const find_method method) noexcept {
        node *lastl{nullptr}, *lastr{nullptr}, *found{nullptr};

        while (current != nullptr && found == nullptr) {
            TRIPLE_COMPARE(compare, k, current->data.first,
                           lastr = current; current = current->left,
                           lastl = current; current = current->right,
                           found = current
            );
        }

        switch (method) {
            case EXACT: return found;
            case LEFT: return NNL(found, lastl);
            case RIGHT: return NNL(found, lastr);
            default: return nullptr;
        }
    }

    /**
     * Traverses the tree from the local root and returns the left-most node
     * (it may be the local root itself if no left children is present)
     * @param n     local root to start from
     * @return      left-most in the tree
     */
    static node* __left_most(node* n) noexcept {
        while (n && n->left) { n = n->left; }
        return n;
    }

    /**
     * Traverses the tree from the local root and returns the right-most node
     * (it may be the local root itself if no right children is present)
     * @param n     local root to start from
     * @return      right-most in the tree
     */
    static node* __right_most(node* n) noexcept {
        while (n && n->right) { n = n->right; }
        return n;
    }

    /**
     * Given a local-root performs a left rotation of the tree below.
     * @param n     local root to rotate
     * @return      return the new local root for this sub tree
     */
    node* __rotate_left(node *n) noexcept {
        node* p = n->parent;
        node* nnew = n->right;
        // Rotate
        if (nnew) {
            CHILD_RIGHT(n, nnew->left);
            CHILD_LEFT(nnew, n);
            CHILD_AS(p, n, nnew);
            REFRESH_DEPTH(n);
            REFRESH_DEPTH(nnew);
        }
        return NNL(nnew, n);
    }

    /**
     * Given a local-root performs a right rotation of the tree below.
     * @param n     local root to rotate
     * @return      return the new local root for this sub tree
     */
    node* __rotate_right(node* n) noexcept {
        node* p = n->parent;
        node* nnew = n->left;
        if (nnew) {
            // Rotate
            CHILD_LEFT(n, nnew->right);
            CHILD_RIGHT(nnew, n);
            CHILD_AS(p, n, nnew);
            REFRESH_DEPTH(n);
            REFRESH_DEPTH(nnew);
        }
        return NNL(nnew, n);
    }

    /**
     * Given a local-root performs rotation if required.
     *
     * ALGORITHM:
     * For each node we keep a ~up to date~ depth count. Assuming dl
     * is the depth of the left branch and dr the left of the right branch
     * When evaluating if a rotation is required:
     *
     * - dl > (dr + 1) : the tree is very left unbalanced perform a right rotation
     * - (dl + 1) < dr : the tree is very right unbalanced perform a left rotation
     *
     * The above algorithm alone does not properly converge due to rotation being
     * symmetric on a zig-zag tree. Hence we keep opening the tree, moving the
     * unbalance to the same direction as the child is child-of-parent. (Eg, left
     * children will tend to be left-unbalanced & right children will tend to be
     * right-unbalanced) This efficiently tends to pack the tree. Still I belive
     * balancing is only heuristic (eg.: I do not believe there is strong depth
     * upper bound guarantee)
     *
     * - dl > dr && n->parent && n->parent->right == n :
     *      the tree is left unbalanced and we are a right child -> right rotate
     * - dl < dr && n->parent && n->parent->left == n
     *      the tree is right unbalanced and we are a left child -> left rotate
     *
     * @param n     local root to rotate
     * @return      return the new local root for this sub tree
     */
    node* __if_required_rotate(node* n) noexcept {
        unsigned int dl = DEPTH_LEFT(n);
        unsigned int dr = DEPTH_RIGHT(n);

#ifdef __DEBUG_BALANCE
        std::cout << "--depths " << n << " " << dl << " " << dr;
#endif
        if ((dl > (dr + 1)) || (dl > dr && n->parent && n->parent->right == n)) {
            // left unbalanced (example: dl = 3, dr = 1)
            // left unbalanced RIGHT CHILD (example: dl = 2, dr = 1)
#ifdef __DEBUG_BALANCE
            std::cout << " --rotate-right" << std::endl;
#endif
            return __rotate_right(n);
        } else if (dr > (dl + 1) || (dl < dr && n->parent && n->parent->left == n)) {
            // right unbalanced (example: dl = 1, dr = 3)
            // right unbalanced LEFT CHILD (example: dl = 1, dr = 2)
#ifdef __DEBUG_BALANCE
            std::cout << " --rotate-left" << std::endl;
#endif
            return __rotate_left(n);
        } else {
            // Else update node
#ifdef __DEBUG_BALANCE
            std::cout << std::endl;
#endif
            n->depth = MAX(dl, dr);
            return n;
        }
    }

    /**
     * Given a node it balances it and all its parents up to the root.
     * @param n     the node to start from
     */
    void __balance_node(node* n) noexcept {
        while (n != nullptr) {
            // `__if_required_rotate` will return the new local root.
            // deep depths has already been updated. The new local root
            // may be the same node `n` or a `n`'s ex-children, in both
            // cases those nodes has been already evaluated.
            n = __if_required_rotate(n)->parent;
        }
    }

    /**
     * Balances the entire tree.
     *
     * ALGORITHM:
     * Performing a deep first iteration (where it is granted LEFT-CHILD,
     * RIGHT-CHILD, PARENT visit order) on the tree and apply `__if_required_rotate`
     * on the node.
     *
     */
    void __balance_tree(/*node* tree*/) noexcept {

        // Deep first iteration strategy
        node* current = root;
        char dir_flag = 0; // 0 = NONE, 1 = UP_FROM_LEFT, 2 = UP_FROM_RIGHT

        while(current != nullptr) {
            if (dir_flag == 0 && current->left) {
                // Go down on the left (nothing visited yet)
                dir_flag = 0;
                current = current->left;
            } else if (dir_flag != 2 && current->right) {
                // Go down on the right (left was visited, but right not)
                dir_flag = 0;
                current = current->right;
            } else {
                // Im a leaf OR my children were already visited

                // Re-balance by evaluating children depths (being the iteration
                // deep-first we have already computed updated depths for the
                // children nodes.

                // The parent node is evaluated after L/R children hence for any
                // type of rotation the new LOCAL ROOT has been evaluated
                // hence continue from the new LOCAL ROOT
                current = __if_required_rotate(current);
                // Go up
                dir_flag = current->parent && current == current->parent->left ? 1 : 2;
                current = current->parent;
            }
        }
    }

    /**
     * Inserts a pair in the tree, returning the node.
     * @param x     pair to be inserted
     * @return      the inserted node or nullptr if key already present
     */
    node* __insert(pair_type&& x) {
        node* parent = nullptr;
        node** handle = &root;

        while (*handle != nullptr) {
            parent = *handle;
            TRIPLE_COMPARE(compare, x.first, parent->data.first,
                           handle = &(parent->left),
                           handle = &(parent->right),
                           return nullptr
            )
        }

        // If here we have an allocable branch
        *handle = new node{parent, x};

#ifdef __EXPERIMENTAL_AUTO_BALANCE
        __balance_node((*handle)->parent);
#endif

        _size ++;
        return *handle;
    }

    /**
     * Extracts a node from the tree by key. The extracted node
     * is completely detached from the tree and should be deleted
     * after use.
     * @param k     the key to search for
     * @return      the node or nullptr if key was not found
     */
    node* __extract(const K& k) noexcept {
        node* n = __find_key(root, k, EXACT);
        if (n == nullptr) return n;

        // Key node was found, take:
        // - right-most in left branch
        // - left-most in right branch
        // them on this node place
        node* p = n->parent;

        if (n->left && n->right) {
            // Multiple branches present
            // Choose left or right based on depths
            node* nnew;
            node* nnew_parent;

            if (n->right->depth > n->left->depth) {
                // Go right
                nnew = __left_most(n->right);
                // extract from tree: It is a left child and has no left child
                // !! If we do not move (eg there are no LEFT children) nnew->parent
                // !! is a RIGHT child
                CHILD_AS(nnew->parent, nnew, nnew->right);
            } else {
                // Go left
                nnew = __right_most(n->left);
                // extract from tree: It is a right child and has no right child
                // !! If we do not move (eg there are no RIGHT children) nnew->parent
                // !! is a RIGHT child
                CHILD_AS(nnew->parent, nnew, nnew->left);
            }

            nnew_parent = nnew->parent;

            // Inplace it in the tree
            CHILD_LEFT(nnew, n->left);
            CHILD_RIGHT(nnew, n->right);
            CHILD_AS(p, n, nnew);
            DETACH(n);

            // Balance starting from the amputation area, both the extracted
            // and the implanted nodes will be balanced

#ifdef __EXPERIMENTAL_AUTO_BALANCE
            __balance_node(nnew_parent);
#endif
        } else {
            // Only one branch present, move it
            node* nnew = n->left ? n->left : n->right;
            CHILD_AS(p, n, nnew);
            DETACH(n);

            // Balance node parents (balance is unlikely required, but the
            // function is used to propagate depths)
#ifdef __EXPERIMENTAL_AUTO_BALANCE
            __balance_node(p);
#endif
        }

        _size--;
        return n;
    }

// API

public:

    // Mimic std::map
    // https://en.cppreference.com/w/cpp/container/map
    using key_type = K;
    using mapped_type = V;
    using value_type = pair_type;
    using key_compare = Compare;

    using iterator = _iterator<node, pair_type>;
    using const_iterator = _iterator<node, const pair_type>;
    using node_type = node;

    // RAII & Copy and move

    bst() noexcept {}

    bst(const bst& src) {
        // Make a copy of the other tree
        root = src.root == nullptr ? nullptr : new node{*src.root};
        _size = src._size;
    }
    bst& operator=(bst const& src) {
        // Self assign guard
        if (this == &src) return *this;
        // Assignment copy
        delete root;
        root = src.root == nullptr ? nullptr : new node{*src.root};
        _size = src._size;
        return *this;
    };

    bst(bst&& src) noexcept:
        root{std::exchange(src.root, nullptr)},
        _size{std::exchange(src._size, 0)}
    { /* steal the tree */ }

    bst& operator=(bst&& src) noexcept {
        delete root;
        root = std::exchange(src.root, nullptr);
        _size = std::exchange(src._size, 0);
        return *this;
    }

    /**
     * Create a map from an iterable iterable source of
     * pair<K,V> values.
     * @tparam Iter
     * @param begin     The iterator
     * @param end       The end() iterator
     */
    template<typename Iter>
    bst(Iter begin, Iter end) {
        while(begin != end) {
            insert(*begin);
            ++begin;
        }
    }

    ~bst() {
#ifdef __DEBUG_BST_RAII
        std::cout << "~bst() size=" << _size << std::endl;
#endif
        delete root; // Delete root node and let RAII do the rest
    }

// MODIFIERS

    /**
     * Inserts a pair in the map. If insertion is successful returns an
     * iterator to the pair and true, else the end() iterator and false.
     * @param x     The pair to be inserted
     * @return      a pair<iterator, bool>
     */
    std::pair<iterator, bool> insert(const pair_type& x) { // ✓ testing
        pair_type t{x}; // Clone the pair
        node* ref = __insert(std::move(t));
        return ref == nullptr ? NOINSERT : std::pair<iterator, bool>{ iterator{root, ref} , true };
    }
    std::pair<iterator, bool> insert(pair_type&& x) { // ✓ testing
        node* ref = __insert(std::move(x));
        return ref == nullptr ? NOINSERT : std::pair<iterator, bool>{ iterator{root, ref} , true };
    }

    /**
     * Inserts multiple pairs in the map. If at least one insertion is
     * successful returns an iterator to the FIRST INSERTED pair and true.
     * If none of the values has been inserted end() and false are returned.
     * @param x     The pair to be inserted
     * @return      a pair<iterator, bool>
     */
    template<class... Types>
    std::pair<iterator, bool> emplace(Types&&... args) { // ✓ testing
        node* ref{nullptr};
        for (auto&& a : { args... }) {
            node* ins = __insert(std::move(a));
            ref = NNL(ref, ins);
        }
        return ref == nullptr ? NOINSERT : std::pair<iterator, bool>{ iterator{root, ref} , true };
    }

    /**
     * Removes a key from the map.
     * @param k     The key to remove
     * @return      The number of values removed
     */
    size_type erase(const K& k) noexcept { // ✓ testing
        node* n = __extract(k);
        delete n;
        return (n == nullptr) ? 0 : 1;
    }

    /**
     * Pops a value from the map returning the value.
     * @param k     The key to remove
     * @return      The value that was associated to the key
     *              or default value for value_type
     */
    value_type pop(const K& k) noexcept { // ✓ testing
        node* n = __extract(k);
        if (n == nullptr) {
            return value_type{};
        } else {
            value_type old = n->data;
            delete n;
            return old;
        }
    }

    /**
     * Removes all the values from the map
     */
    void clear() noexcept { // ✓ testing
        delete root;
        root = nullptr;
        _size = 0;
    }

    /**
     * Balances the tree
     */
    void balance() noexcept { // ✓ testing
        __balance_tree();
    }

// GETTERS

    /**
     * Weather the map contains a given key
     * @param k     The key to search for
     * @return      True if value is present
     */
    bool has(const K& k) noexcept { // ✓ testing
        return __find_key(root, k, EXACT) != nullptr;
    }

    /**
     * Searches for a key, if the key is present
     * an iterator that starts at that key is returned
     * else end() is returned.
     * @param k     The key to search for
     * @return      The iterator
     */
    iterator find(const K& k) noexcept { // ✓ testing
        node* found = __find_key(root, k, EXACT);
        return found == nullptr ? end() : iterator{root, found};
    }
    const_iterator find(const K& k) const noexcept {
        node* found = __find_key(root, k, EXACT);
        return found == nullptr ? end() : const_iterator{root, found};
    }

    /**
     * Returns the size of the map O(1)
     * @return      The size of the map
     */
    size_type size() noexcept { return _size; }

    /**
     * Check weather the map is empty O(1)
     * @return      True if the map is empty
     */
    bool empty() noexcept { return _size == 0; }

    /**
     * Returns the current depth of the map O(1)
     * @return      The depth of the map
     */
    unsigned char depth() noexcept {
        return root ? root->depth + 1: 0;
    }

    /**
     * (specs from std::map):
     * Allows for easy lookup with the subscript ( @c [] ) operator.  Returns
     * data associated with the key specified in subscript.  If the key does
     * not exist, a pair with that key is created using default values, which
     * is then returned.
     *
     * @param k     The key for which data should be retrieved
     * @return      A reference to the data related to the key
     */
    V& operator[](const K& k) { // ✓ testing
        iterator _i = find(k);
        if (_i == end()) {
            _i = insert(pair_type{k, V{}}).first;
        }
        return _i->second;
    }
    V& operator[](K&& k) { // ✓ testing
        iterator _i = find(k);
        if (_i == end()) {
            _i = insert(pair_type{k, V{}}).first;
        }
        return _i->second;
    }

    /**
     * Returns an iterator to a slice of the map. The slice will start
     * at the first key greater or equal to lower and will end with the
     * last key lower or equal to upper.
     *
     * @param lower     The lower inclusive bound
     * @param upper     The upper inclusive bound
     * @return          The iterator
     */
    iterator operator()(const K& lower, const K& upper) noexcept {
        if (compare(upper, lower)) return iterator{};
        // Check that the RIGHT neighbour is not greater than upper
        node* lower_node = __find_key(root, lower, RIGHT);
        if (compare(upper, lower_node->data.first)) return iterator{};
        // Check that the LEFT neighbour is not lower than lower
        node* upper_node = __find_key(root, upper, LEFT);
        if (compare(upper_node->data.first, lower)) return iterator{};
        // Ok
        return iterator{root, lower_node, upper_node};
    }
    const_iterator operator()(const K& lower, const K& upper) const noexcept {
        if (compare(upper, lower)) return const_iterator{};
        // Check that the RIGHT neighbour is not greater than upper
        node* lower_node = __find_key(root, lower, RIGHT);
        if (compare(upper, lower_node->data.first)) return const_iterator{};
        // Check that the LEFT neighbour is not lower than lower
        node* upper_node = __find_key(root, upper, LEFT);
        if (compare(upper_node->data.first, lower)) return const_iterator{};
        // Ok
        return iterator{root, lower_node, upper_node};
    }

// ITERATORS

    /**
     * An iterator of the map elements starting with the lower key
     * @return          The iterator
     */
    iterator begin() noexcept {
        return iterator{root, __left_most(root)};
    }
    const_iterator begin() const noexcept {
        return const_iterator{root, __left_most(root)};
    };
    const_iterator cbegin() const noexcept {
        return const_iterator{root, __left_most(root)};
    };

    // https://www.cplusplus.com/reference/map/map/end/
    /**
     * The end iterator for this map (a simple nullptr reference)
     * @return          The iterator
     */
    iterator end() noexcept {
        return iterator{root, nullptr};
    }
    const_iterator end() const noexcept {
        return const_iterator{root, nullptr};
    }
    const_iterator cend() const noexcept {
        return const_iterator{root, nullptr};
    }

// OTHER

    friend bool operator==(const bst& a, const bst& b) noexcept { // ✓ testing
        return a.root == b.root;
    }
    friend bool operator!=(const bst& a, const bst& b) noexcept { // ✓ testing
        return a.root != b.root;
    }

    /**
     * Prints the json-style representation of the map
     */
    friend std::ostream& operator<<(std::ostream& os, const bst& x) {
        os << "size: " << x.size() << "\n";
        os << "{ ";
        bool nfirst = false;
        for (auto&& kv: x) {
            if (nfirst) {
                os << ", ";
            }
            nfirst = true;
            os << kv.first << ":" << kv.second;
        }
        os << " }" << std::endl;
        return os;
    }

    // DEBUG

private:

    void __print_tree(std::ostream& os, std::string&& pref, std::string&& pref_rest, node* from);

public:

    /**
     * Prints the tree structure on a std::ostream
     */
    void print_tree(std::ostream& os);

    /**
     * Prints the tree structure on the std::cout
     */
    void print_tree();

    /**
     * Prints the tree info on a std::ostream
     */
    void tree_info(std::ostream& os);

    /**
     * Prints the tree info on the std::cout
     */
    void tree_info();

};


template <typename K, typename V>
struct node {

    node* parent{nullptr};
    node* left{nullptr};
    node* right{nullptr};
    unsigned char depth;
    std::pair<const K, V> data;

    explicit node(node* parent, std::pair<const K, V>& pair) noexcept:
            parent{parent},
            depth{0},
            data{pair}
    {
#ifdef __DEBUG_NODE_RAII
        std::cout << "Allocated: " << pair.first << std::endl;
#endif
    };
    explicit node(node* parent, std::pair<const K, V>&& pair) noexcept:
            parent{parent},
            depth{0},
            data{std::move(pair)}
    {
#ifdef __DEBUG_NODE_RAII
        std::cout << "Allocated: " << pair.first << std::endl;
#endif
    };

    explicit node(node& src):
            parent{src.parent},
            left{src.left == nullptr ? nullptr : new node{*src.left}},
            right{src.right == nullptr ? nullptr : new node{*src.right}},
            depth{src.depth},
            data{src.data}
    {
#ifdef __DEBUG_NODE_RAII
        std::cout << "Allocated: copy" << std::endl;
#endif
    }

    ~node() {
#ifdef __DEBUG_NODE_RAII
        std::cout << "Destroying: " << data.first << std::endl;
#endif
        // Custom RAII to propagate tree deletion
        delete this->left;
        delete this->right;
    }
};


template<typename elem_type, typename VT>
class _iterator {

    using elem_ptr = elem_type*;
    elem_ptr root{nullptr};  // used to inexpensively recover from upper()
    elem_ptr current{nullptr};

    // For range
    elem_ptr lower{nullptr};
    elem_ptr upper{nullptr};

    // Helper methods

    static elem_type* __left_most(elem_type* n) noexcept {
        while (n && n->left) { n = n->left; }
        return n;
    }

    static elem_type* __right_most(elem_type* n) noexcept {
        while (n && n->right) { n = n->right; }
        return n;
    }

public:

    // https://en.cppreference.com/w/cpp/named_req/Iterator
    // https://internalpointers.com/post/writing-custom-iterators-modern-cpp
    explicit _iterator() noexcept {}
    explicit _iterator(elem_ptr root) noexcept: root{root} { }
    _iterator(elem_ptr root, elem_ptr start) noexcept: root{root}, current{start} { }
    _iterator(elem_ptr root, elem_ptr lower, elem_ptr upper) noexcept:
            root{root}, current{lower}, lower{lower}, upper{upper} {}
    _iterator(elem_ptr root, elem_ptr current, elem_ptr lower, elem_ptr upper) noexcept:
            root{root}, current{current}, lower{lower}, upper{upper} {}

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = VT ;
    using pointer = VT*;
    using reference = VT&;

    reference operator*() {
#ifdef __DEBUG_ITERATOR
        std::cout << "*: " << current << std::endl;
#endif
//        assert(current != nullptr, "Can not dereference end iterator");
        return static_cast<reference>(current->data);
    }
    const VT& operator*() const {
#ifdef __DEBUG_ITERATOR
        std::cout << "const *: " << current << std::endl;
#endif
//        assert(current != nullptr, "Can not dereference end iterator");
        return const_cast<const VT&>(current->data);
    }

    pointer operator->() {
        return &current->data;
    }

    _iterator& operator++() noexcept {
#ifdef __DEBUG_ITERATOR
        std::cout << "++: " << current << std::endl;
#endif
        if (current == nullptr) {
#ifdef __ITERATOR_RECOVERABLE
            // !! not sure what is the convention for bi-directionality
            current = NNL(lower, __left_most(root));
#endif
        } else if (current == upper) {
            // We reached UPPER range boundary
            current = nullptr;
        } else if (current->right) {
            // Go down on the right branch
            current = __left_most(current->right);
        } else if (current->parent) {
            // Nothing left below
            // Traverse the parent tree upward till there is a parent of whom the
            // child was a left-child, otherwise we reached the right-most branch
            elem_ptr tmp = current;
            current = current->parent;
            // Traverse
            while (current != nullptr) {
                if (current->left == tmp) {
                    // I was the left child; it is turn of the parent
                    break;
                }
                // I was the right child (assuming structure is not corrupted)
                tmp = current;
                current = current->parent;
            }
        } else {
            // We reached the upper
            current = nullptr;
        }

        return *this;
    }

    _iterator& operator--() noexcept {
#ifdef __DEBUG_ITERATOR
        std::cout << "--: " << current << std::endl;
#endif

        if (current == nullptr) {
#ifdef __ITERATOR_RECOVERABLE
            // !! not sure what is the convention for bi-directionality
            current = NNL(upper, __right_most(root));
#endif
        } else if (current == lower) {
            // We reached LOWER range boundary
#ifdef __ITERATOR_LOWER_END
            current = nullptr;
#endif
        } else if (current->left) {
            current = __right_most(current->left);
        } else if (current->parent) {
            // Nothing left below
            // Traverse up to a node of whom current is a right-child, else
            // we reached the left-most branch. -- will do nothing from here
            // (will remain on begin() pointer)
            elem_ptr tmp = current, parent = current->parent;
            while (parent != nullptr) {
                if (parent->right == tmp) {
                    current = parent;
                    break;
                }
                tmp = parent;
                parent = parent->parent;
            }
#ifdef __ITERATOR_LOWER_END
            // !! not sure what is the convention for bi-directionality
            // Cause -- to become end()
            current = parent;
#endif
        }

        return *this;
    }

    friend bool operator==(const _iterator& a, const _iterator& b) noexcept {
#ifdef __DEBUG_ITERATOR
        std::cout << "==: " << a.current << " " << b.current << std::endl;
#endif
        return a.current == b.current;
    }
    friend bool operator!=(const _iterator& a, const _iterator& b) noexcept {
#ifdef __DEBUG_ITERATOR
        std::cout << "!=: " << a.current << " " << b.current << std::endl;
#endif
        return a.current != b.current;
    }
};


template <typename K, typename V, typename Compare, typename Size>
void bst<K, V, Compare, Size>::__print_tree(std::ostream& os, std::string&& pref, std::string&& pref_rest, node* from) {
    if (from == nullptr) {
        os << pref << "(empty)\n";
    } else {
        os << pref
           << from
           << " [depth=" << from->depth
           << ", parent=" << from->parent << ", left=" << from->left << ", right=" << from->right << "]"
           << " (" << from->data.first << ":" << from->data.second << ")\n";

        std::stringstream ff_s;
        ff_s << pref_rest   << "|->L ";
        std::stringstream fr_s;
        fr_s << pref_rest   << "|    ";
        std::stringstream rf_s;
        rf_s << pref_rest   << "-->R ";
        std::stringstream rr_s;
        rr_s << pref_rest   << "     ";

        __print_tree(os, ff_s.str(), fr_s.str(), from->left);
        __print_tree(os, rf_s.str(), rr_s.str(), from->right);
    }
}

template <typename K, typename V, typename Compare, typename Size>
void bst<K, V, Compare, Size>::print_tree(std::ostream& os) {
    os << "Size: " << _size << "\n";
    __print_tree(os, "", "", root);
    os << std::endl;
}

template <typename K, typename V, typename Compare, typename Size>
void bst<K, V, Compare, Size>::print_tree() {
    print_tree(std::cout);
}

template <typename K, typename V, typename Compare, typename Size>
void bst<K, V, Compare, Size>::tree_info(std::ostream& os) {
    os << "bst{size=" << _size << ", root=" << root << "}\n";
}

template <typename K, typename V, typename Compare, typename Size>
void bst<K, V, Compare, Size>::tree_info() {
    tree_info(std::cout);
}
