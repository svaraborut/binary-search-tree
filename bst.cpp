#pragma once

#include <iostream>
#include <utility>
#include <sstream>

//#define __DEBUG_ITERATOR
//#define __DEBUG_ITERATOR_2
//#define __DEBUG_NODE_RAII
//#define __DEBUG_BST_RAII
//#define __DEBUG_BALANCE
#define PV(name, node) \
    std::cout << name << " " << node \
    << " [parent=" << node->parent << ", l=" << node->left << ", r=" << node->right << "]" << std::endl


#define __EXPERIMENTAL_AUTO_BALANCE

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)
#define NNL(_1, _2) (_1) != nullptr ? (_1) : (_2)

#define TRIPLE_COMPARE(cmp, a, b, less, gt, eq) \
    if (cmp(a, b)) { less; }                    \
    else if (cmp(b, a)) { gt; }                 \
    else { eq; }

#define NOINSERT std::pair<iterator, bool>{ iterator{}, false }

// Make the node become the left child of the parent
#define CHILD_LEFT(p, c)    \
    if (p) (p)->left = c;   \
    if (c) (c)->parent = p;

// Make the node become the right child of the parent
#define CHILD_RIGHT(p, c)   \
    if (p) (p)->right = c;  \
    if (c) (c)->parent = p;

// Replace parents child
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

#define DEPTH_LEFT(node) (node)->left ? ((node)->left->depth + 1) : 0
#define DEPTH_RIGHT(node) (node)->right ? ((node)->right->depth + 1) : 0
#define REFRESH_DEPTH(node) (node)->depth = MAX(DEPTH_LEFT(node), DEPTH_RIGHT(node))

#define DETACH(node) (node)->left = nullptr; (node)->right = nullptr;


template <typename K, typename V, typename Compare = std::less<K>>
class bst {

    // Definitions

    Compare compare;

    struct node;

    using node_pointer = node*;
    using pair_type = std::pair<const K, V>;
    using size_type = std::size_t;

    node* root{nullptr};
    size_type _size{0};

// ITERATOR

    template<typename value_type>
    class _iterator;

    template<typename value_type>
    class _range_iterator;

    // INNER
    enum find_method{EXACT, LEFT, RIGHT};

    node* __find_key(node* current, const K& k, const find_method method) {
        node_pointer lastl{nullptr}, lastr{nullptr}, found{nullptr};

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

    static node* __left_most(node* n) {
        while (n && n->left) { n = n->left; }
        return n;
    }

    static node* __right_most(node* n) {
        while (n && n->right) { n = n->right; }
        return n;
    }

    // Rotate left and return the up-stepping node
    // Returning the new LOCAL ROOT
    node* __rotate_left(node *n) {
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

    // Rotate right and return the up-stepping node
    // Returning the new LOCAL ROOT
    node* __rotate_right(node* n) {
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

    node* __evaluate_rotation(node* n) {
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

    void __balance_node(node* n) {
        while (n != nullptr) {
            // `__evaluate_rotation` will return the new local root.
            // deep depths has already been updated. The new local root
            // may be the same node `n` or a `n`'s ex-children, in both
            // cases those nodes has been already evaluated.
            n = __evaluate_rotation(n)->parent;
        }
    }

    node* __balance_tree(node* tree) {

        // Deep first iteration strategy
        node* current = tree;
        char dir_flag = 0; // 0 = NONE, 1 = UP_FROM_LEFT, 2 = UP_FROM_RIGHT

        while(current != nullptr) {
//            std::cout << current << std::endl;

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
//                std::cout << current << " = "
//                          << " (" << current->data.first << ") "
//                          << " " << current->parent << std::endl;

                // The parent node is evaluated after L/R children hence for any
                // type of rotation the new LOCAL ROOT has been evaluated
                // hence continue from the new LOCAL ROOT
                current = __evaluate_rotation(current);
                // Go up
                dir_flag = current->parent && current == current->parent->left ? 1 : 2;
                current = current->parent;
            }
        }

        return root;
    }

    node* __insert(pair_type&& x) {
        bool debug = x.first == 3058;

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

    node* __extract(const K& k) {
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
//                CHILD_LEFT(nnew->parent, nnew->right);
//                std::cout << "CHILD_LEFT" << std::endl;
            } else {
                // Go left
                nnew = __right_most(n->left);
                // extract from tree: It is a right child and has no right child
                // !! If we do not move (eg there are no RIGHT children) nnew->parent
                // !! is a RIGHT child
                CHILD_AS(nnew->parent, nnew, nnew->left);
//                CHILD_RIGHT(nnew->parent, nnew->left);
//                std::cout << "CHILD_RIGHT" << std::endl;
            }

            nnew_parent = nnew->parent;

//            PV("n ->", n);
//            PV("p ->", p);
//            PV("nnew ->", nnew);
//            PV("nnew_parent ->", nnew_parent);

            // Inplace it in the tree
            CHILD_LEFT(nnew, n->left);
            CHILD_RIGHT(nnew, n->right);
            CHILD_AS(p, n, nnew);
            DETACH(n);

            // Balance starting from the amputation area, both the extracted
            // and the implaced nodes will be balanced

#ifdef __EXPERIMENTAL_AUTO_BALANCE
            __balance_node(nnew_parent);
#endif
        } else {
            // Only one branch present, move it
            node* nnew = n->left ? n->left : n->right;

//            std::cout << "n=" << n << std::endl;
//            std::cout << "p=" << p << std::endl;
//            std::cout << "nnew=" << nnew << std::endl;

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

    using iterator = _iterator<pair_type>;
    using const_iterator = _iterator<const pair_type>;
    using range_iterator = _range_iterator<pair_type>;
    using const_range_iterator = _iterator<const pair_type>;
    using value_type = pair_type;
    using node_type = node;

    // RAII & Copy and move

    bst() noexcept {}

    bst(const bst& src) {
        // Make a copy of the other tree
        root = src.root == nullptr ? nullptr : new node{*src.root};
        _size = src._size;
    }
    bst& operator=(bst const& src) = default;

    bst(bst&& src) noexcept:
        root{std::exchange(src.root, nullptr)},
        _size{std::exchange(src._size, 0)}
    { /* Swap tree with the other class */ }

    bst& operator=(bst&& src) noexcept {
        root = std::exchange(src.root, nullptr);
        _size = std::exchange(src._size, 0);
        return *this;
    }

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
        // Delete root node and let RAII do the rest
        delete root;
    }

// MODIFIERS

    std::pair<iterator, bool> insert(const pair_type& x) { // ✓ testing
        pair_type t{x}; // Clone the pair
        node* ref = __insert(std::move(t));
        return ref == nullptr ? NOINSERT : std::pair<iterator, bool>{ iterator{root, ref} , true };
    }
    std::pair<iterator, bool> insert(pair_type&& x) { // ✓ testing
        node* ref = __insert(std::move(x));
        return ref == nullptr ? NOINSERT : std::pair<iterator, bool>{ iterator{root, ref} , true };
    }

    template<class... Types>
    std::pair<iterator, bool> emplace(Types&&... args) { // ✓ testing
        // todo : this logic is not so intuitive
        std::pair<iterator, bool> last = NOINSERT;
        for (auto&& a : { args... }) last = insert(a);
        return last;
    }

    size_type erase(const K& k) { // ✓ testing
        node* n = __extract(k);
        delete n;
        return (n == nullptr) ? 0 : 1;
    }

    value_type pop(const K& k) { // ✓ testing
        node* n = __extract(k);
        if (n == nullptr) {
            return value_type{};
        } else {
            value_type old = n->data;
            delete n;
            return old;
        }
    }

    // Clear
    void clear() { // ✓ testing
        _size = 0;
        root = nullptr;
    }

    // Balance
    void balance() { // ✓ testing
        __balance_tree(root);
    }

// GETTERS

    // Contains
    bool has(const K& k) { // ✓ testing
        return __find_key(root, k, EXACT) != nullptr;
    }

    // Find
    iterator find(const K& k) { // ✓ testing
        node* found = __find_key(root, k, EXACT);
        return found == nullptr ? end() : iterator{root, found};
    }
    const_iterator find(const K& k) const {
        node* found = __find_key(root, k, EXACT);
        return found == nullptr ? end() : const_iterator{root, found};
    }

    size_type size() { return _size; }
    bool empty() { return _size == 0; }
    unsigned int depth() {
        return root ? root->depth + 1: 0;
    }

    /**
     * @param k     The key for which data should be retrieved
     * @return      A reference to the data related to the key
     *
     * (specs from std::map):
     * Allows for easy lookup with the subscript ( @c [] ) operator.  Returns
     * data associated with the key specified in subscript.  If the key does
     * not exist, a pair with that key is created using default values, which
     * is then returned.
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

    range_iterator operator()(const K& lower, const K& upper) {
        if (compare(upper, lower)) return range_iterator{};
        return range_iterator{
            root,
            __find_key(root, lower, RIGHT),
            __find_key(root, upper, LEFT)
        };
    }
    const_range_iterator operator()(const K& lower, const K& upper) const {
        if (compare(upper, lower)) return const_range_iterator{};
        return const_range_iterator{
                root,
                __find_key(root, lower, RIGHT),
                __find_key(root, upper, LEFT)
        };
    }


// ITERATORS

    // Begin
    iterator begin() {
        return iterator{root, __left_most(root)};
    }
    const_iterator begin() const {
        return const_iterator{root, __left_most(root)};
    };
    const_iterator cbegin() const {
        return const_iterator{root, __left_most(root)};
    };

    // End
    // https://www.cplusplus.com/reference/map/map/end/
    iterator end() {
        return iterator{root, nullptr};
    }
    const_iterator end() const {
        return const_iterator{root, nullptr};
    }
    const_iterator cend() const {
        return const_iterator{root, nullptr};
    }

// OTHER

    friend bool operator==(const bst& a, const bst& b) noexcept { // ✓ testing
        return a.root == b.root;
    }
    friend bool operator!=(const bst& a, const bst& b) noexcept { // ✓ testing
        return a.root != b.root;
    }

    // todo : const here caues issues
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

    void __print_tree(std::ostream& os, std::string&& pref, std::string&& pref_rest, node* from) {

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

public:

    void print_tree() {
        std::cout << "Size: " << _size << "\n";
        __print_tree(std::cout, "", "", root);
        std::cout << std::endl;
    }

    void info_tree() {
        std::cout << "bst{size=" << _size << ", root=" << root << "}\n";
    }

};

template <typename K, typename V, typename Compare>
struct bst<K, V, Compare>::node {

    node* parent{nullptr};
    node* left{nullptr};
    node* right{nullptr};
    unsigned int depth;
    std::pair<const K, V> data;

    explicit node(node* parent, std::pair<const K, V>& pair):
            parent{parent},
            data{pair},
            depth{0}
    {
#ifdef __DEBUG_NODE_RAII
        std::cout << "Allocated: " << pair.first << std::endl;
#endif
    };
    explicit node(node* parent, std::pair<const K, V>&& pair):
            parent{parent},
            data{pair},
            depth{0}
    {
#ifdef __DEBUG_NODE_RAII
        std::cout << "Allocated: " << pair.first << std::endl;
#endif
    };

    node(node& src):
            parent{src.parent},
            left{src.left == nullptr ? nullptr : new node{*src.left}},
            right{src.right == nullptr ? nullptr : new node{*src.right}},
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
        // Custom RAII to prevent parent deletion
        delete this->left;
        delete this->right;
    }
};

template <typename K, typename V, typename Compare>
template<typename VT>
class bst<K, V, Compare>::_iterator {

//protected:

    using elem_ref = bst<K, V, Compare>::node*;
    elem_ref root;  // used to inexpensively recover from upper()
    elem_ref current;

    // Expose private to bst members
    friend bst<K, V, Compare>;

    explicit _iterator(elem_ref root) noexcept: root{root}, current{nullptr} { }
    _iterator(elem_ref root, elem_ref start) noexcept: root{root}, current{start} { }

public:

    _iterator() noexcept: root{nullptr}, current{nullptr} { }
    // https://en.cppreference.com/w/cpp/named_req/Iterator
    // https://internalpointers.com/post/writing-custom-iterators-modern-cpp


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
        return &**this;
    }

    _iterator& operator++() noexcept {
#ifdef __DEBUG_ITERATOR
        std::cout << "++: " << current << std::endl;
#endif
        if (current == nullptr) {
            // todo : not sure how to deal with bi-directionality
            ;  // noop
        } if (current->right) {
            // Go down on the right branch
            current = __left_most(current->right);
        } else if (current->parent) {
            // Nothing left below
            // Traverse the parent tree upward till there is a parent of whom the
            // child was a left-child, otherwise we reached the right-most branch
            elem_ref tmp = current;
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
            // todo : not sure how to deal with bi-directionality
            // Attempt to recover from end()
            current = __right_most(root);
        } else if (current->left) {
            current = __right_most(current->left);
        } else if (current->parent) {
            // Nothing left below
            // Traverse up to a node of whom current is a right-child, else
            // we reached the left-most branch. -- will do nothing from here
            // (will remain on begin() pointer)
            elem_ref tmp = current, parent = current->parent;
            while (parent != nullptr) {
                if (parent->right == tmp) {
                    current = parent;
                    break;
                }
                tmp = parent;
                parent = parent->parent;
            }
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

template <typename K, typename V, typename Compare>
template<typename VT>
class bst<K, V, Compare>::_range_iterator: public bst<K, V, Compare>::_iterator<VT>  {

    using elem_ref = bst<K, V, Compare>::node*;
    using _base = bst<K, V, Compare>::_iterator<VT>;

    elem_ref lower;
    elem_ref upper;

    // Expose private to bst members
    friend bst<K, V, Compare>;

    _range_iterator(elem_ref root, elem_ref lower, elem_ref upper) noexcept:
            _base{root, lower}, lower{lower}, upper{upper} {}
    _range_iterator(elem_ref root, elem_ref start, elem_ref lower, elem_ref upper) noexcept:
        _base{root, start}, lower{lower}, upper{upper} {}

public:

    _range_iterator() noexcept: _base{}, lower{nullptr}, upper{nullptr} {}
    _range_iterator(const _base& src): _base{src}, lower{nullptr}, upper{nullptr} {}

    _range_iterator& operator++() noexcept {
#ifdef __DEBUG_ITERATOR_2
        std::cout << "_range_iterator::++: " << current << std::endl;
#endif
        if (_base::current == nullptr) {
            // todo : not sure how to deal with bi-directionality
            ; // noop
        } else if (_base::current == upper) {
            // We reached UPPER range end
            _base::current = nullptr;
        } else {
            _base::operator++();
        }

        return *this;
    }

    _range_iterator& operator--() noexcept {
#ifdef __DEBUG_ITERATOR_2
        std::cout << "_range_iterator::--: " << current << std::endl;
#endif

        if (_base::current == nullptr) {
            // todo : not sure how to deal with bi-directionality
            // Recover from end()
            _base::current = upper;
        } else if (_base::current == lower) {
            // We reached LOWER range end
            ; // noop
        } else {
            _base::operator--();
        }

        return *this;
    }

};
