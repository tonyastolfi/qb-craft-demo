// Implementation of StringTrie and supporting data structures.
//
// A StringTrie<T> is a key-value associative container that maps 8-bit char
// strings onto values of type `T`.  Like `std::multimap`, it allows for
// multiple values to be stored per string key.
//
// Given the constrains of the Craft Demo exercise, some trade-offs
// have been made in favor of simpler code at the cost of space
// efficiency.  In particular, it is not a compressed trie; in fact,
// no attempt is made to save space by storing fewer outbound child
// pointers for trie nodes with small branching factors.
//
// Given more time to work on the implementation, this would be the
// major shortcoming to address, as it not only affects total space
// usage and setup/teardown times, but also hurts certain query
// workloads due to suboptimal memory cache locality.  This is
// particularly pronounced for queries on string patterns what are
// very common in the indexed corpus, where initial benchmark results
// seem to indicate that the cost of walking a large subtree of trie
// nodes to collect all the results makes this operation sometimes not
// much better than a naive linear scan.  However, the general suffix
// trie approach is validated by observed query speedupsof 2-5x on
// substrings that are relatively infrequent, suggesting that even in
// its suboptimal form, this design shows promise.
//
// There are well-known algorithms
// (cf. https://web.stanford.edu/~mjkay/gusfield.pdf) for doing suffix
// trie construction in time linear to the size of all input strings,
// but this implementation, to save coding time, uses the naive
// approach of simply inserting all suffixes of a given string.  Space
// complexity, in addition to being suboptimal in the ways described
// above, is O(|keys| * sizeof(T)); therefore it is intended for `T`
// types of constant size (primary integer key, pointer, etc.)
//
#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

//------------------------------------------------------------------------------

// Represents the set of non-null child branches for a trie node.  The branching
// factor is fixed at 2^8 == 256.  This class extends the functionality of
// `std::bitset<256>` by providing a fast method for enumerating the non-zero
// bit indices based on GCC/Clang compiler intrinsics (at the cost of some
// portability).
//
class BranchSet {
private:
  std::array<std::uint64_t, 4> bits_{{0, 0, 0, 0}};

  static constexpr std::uint64_t mask(int pos) { return 1ull << (pos % 64); }

public:
  // Sets the bit at `pos` to 0 (false) or 1 (true) depending on `value`.
  //
  void set(int pos, bool value) {
    if (value) {
      bits_[pos / 64] |= mask(pos);
    } else {
      bits_[pos / 64] &= ~mask(pos);
    }
  }

  // Returns true iff the bit at `pos` is set to 1.
  //
  bool test(int pos) const { return (bits_[pos / 64] & mask(pos)) != 0ull; }

  // Given a callable `fn` of type `Fn`, which takes an `int` as its only arg
  // (return value is ignored), invokes `fn` on the indices of all bits set to
  // 1.
  //
  template <typename Fn /* void(int index) */> void for_each(Fn &&fn) {
    for (int i = 0; i < 4; ++i) {
      const int base = i * 64;
      std::uint64_t chunk = bits_[i];
      while (chunk != 0) {
        int next = __builtin_ctzll(chunk);
        fn(next + base);
        chunk &= ~(1ull << next);
      }
    }
  }
};

//------------------------------------------------------------------------------

// Trie mapping 8-bit char strings to values of type `T`.
//
template <typename T> class StringTrie {
private:
  struct Node {
    // The set of all non-null child branches.
    //
    BranchSet active;

    // The actual child node pointers, including nulls; the array index is the
    // char value of the next character in the string stored by the child node.
    //
    std::array<Node *, 256> branch; // Originally used std::unique_ptr here but
                                    // because most branches are null,
                                    // destructing a trie was very slow!

    // The values stored at this node; i.e., the values assoicated with the
    // string whose path from the root of the trie leads to `this`.
    //
    std::vector<T> values;

    // Disable copying so we enforce unique ownership of child nodes.
    //
    Node() = default;
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    // Custom dtor to only iterate over branches we actually have to recursively
    // delete.
    //
    ~Node() noexcept {
      active.for_each([&](int i) {
        assert(branch[i] != nullptr);
        delete branch[i];
      });
    }

    // Returns true iff this node has a child corresponding to the given char
    // value.
    //
    bool has_branch(int ch) { return active.test(ch); }

    // Invokes `fn` for each value stored at this node.
    //
    template <typename Fn /* void(const T &) */> void visit_values(Fn &&fn) {
      for (const T &v : values) {
        fn(v);
      }
    }

    // Invokes `fn` for each value stored at this node and all child nodes; used
    // for substring/prefix matching.
    //
    template <typename Fn /* void(const T &) */> void visit_recursive(Fn &&fn) {
      visit_values(fn);
      active.for_each([&](int i) {
        assert(branch[i] != nullptr);
        branch[i]->visit_recursive(fn);
      });
    }
  };

  // Searches from the root of the trie for a match to `key`.  If no such key is
  // found, behavior depends on the value of `create`:
  //
  //  - create=true: a new path is created for the missing portion of `key`
  //  - create=false: nullptr is returned
  //
  Node *find_node(std::string_view key, bool create) {
    Node *node = &root_;
    while (!key.empty()) {
      const int ch = (unsigned char)key.front();
      if (!node->has_branch(ch)) {
        if (!create) {
          return nullptr;
        }
        node->active.set(ch, true);
        node->branch[ch] = new Node();
      }
      node = node->branch[ch];
      key.remove_prefix(1);
    }
    return node;
  }

  // The root of the trie.  Values (`T`) stored here are associated with the
  // empty string.
  //
  Node root_;

public:
  // TODO - to same coding time for this exercise, STL-container style copy
  // semantics are disabled; we could implement these.

  StringTrie() = default;
  StringTrie(const StringTrie &) = delete;
  StringTrie &operator=(const StringTrie &) = delete;

  //================================

  // Inserts `value` under the given `key`.  This operation always creates a new
  // mapping in the trie (because this container has multimap-like semantics).
  //
  // Complexity: O(key.length())
  //
  void insert(std::string_view key, const T &value) {
    Node *node = find_node(key, /*create=*/true);
    node->values.emplace_back(value);
  }

  // Inserts `value` under all the suffixes of key (including key itself).
  //
  void insert_suffixes(std::string_view key, const T &value) {
    while (!key.empty()) {
      insert(key, value);
      key.remove_prefix(1);
    }
  }

  // Invokes `fn` for each mapped value whose key matches `key` exactly.
  //
  template <typename Fn /* void(const T &) */>
  void for_each_exact(std::string_view key, Fn &&fn) {
    Node *node = find_node(key, /*create=*/false);
    if (!node) {
      return;
    }
    node->visit_values(fn);
  }

  // Invokes `fn` for each mapped value whose key starts with `key_prefix`.
  //
  template <typename Fn /* void(const T &) */>
  void for_each_prefix(std::string_view key_prefix, Fn &&fn) {
    Node *node = find_node(key_prefix, /*create=*/false);
    if (!node) {
      return;
    }
    node->visit_recursive(fn);
  }
};
