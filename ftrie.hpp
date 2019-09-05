#ifndef __FTRIE_H__
#define __FTRIE_H__

#include <vector>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <vector>
#include <cmath>

template<unsigned AS, typename OFFSET=unsigned>
class ftrie {
protected:
  enum STATE { EMPTY, INTERNAL, FINAL };
  struct node {
    STATE  state;
    OFFSET edges[AS];
    node(STATE s = EMPTY)
      : state(s)
    {
      clear();
    }

    void clear() {
      memset(edges, '\0', sizeof(OFFSET)*AS);
    }
  };
  static_assert(std::is_nothrow_move_constructible<node>::value,
                "That's a BUG. Node should be nothrow move constructible");
  static_assert(AS > 0,
                "Alphabet size cannot be 0");

  static constexpr OFFSET root_offset = 0; // Guard value. Node containing the root node
  static constexpr OFFSET final_offset = 1; // Guard value. Node containing the FINAL state
  const std::string       alphabet;
  const std::vector<int>  translate;
  OFFSET                  empty_head; // head of list of empty nodes
  std::vector<node>       nodes;      // vector of nodes creating the trie

  static std::vector<int> create_translate(const char* a, size_t s) {
    if(s != AS)
      throw std::range_error("Alphabet of wrong length");
    std::vector<int> res(256, -1);
    for(size_t i = 0; i < s; ++i)
      res[a[i]] = i;
    return res;
  }

  static std::string read_alphabet(std::istream& is) {
    std::string res;
    std::getline(is, res);
    return res;
  }

public:
  enum FOUND { NOT, HAS_PREFIX, PREFIX_OF };

  ftrie(const char* a, size_t s)
    : alphabet(a, s)
    , translate(create_translate(a, s))
    , empty_head(0)
  {
    nodes.emplace_back(INTERNAL);
    nodes.emplace_back(FINAL);
  }
  ftrie(const char* a)
    : ftrie(a, strlen(a))
  { }
  ftrie(const std::string& s)
    : ftrie(s.c_str(), s.size())
  { }

  ftrie(std::istream& is)
    : ftrie(read_alphabet(is))
  {
    is >> empty_head;
    if(!is.good()) throw std::range_error("Incomplete ftrie file");

    nodes.clear();
    node n;
    OFFSET cur = 0;
    for( ; true; ++cur) {
      unsigned s;
      is >> s;
      if(s > FINAL) throw std::range_error("Invalid node state");
      n.state = static_cast<STATE>(s);
      if(cur == 0 && n.state != INTERNAL)
        throw std::range_error("Root node must have INTERNAL state");
      if(cur == 1 && n.state != FINAL)
        throw std::range_error("Guard node must have FINAL state");
      for(unsigned i = 0; i < AS; ++i)
        is >> n.edges[i];
      if(is.eof()) return;
      if(!is.good()) throw std::range_error("Incomplete ftrie file");
      nodes.push_back(n);
    }
  }

protected:
  // Recursively delete an unused subtree and add the nodes to the
  // empty list for future reuse
  void delete_subtree(OFFSET start) {
    if(nodes[start].state == FINAL) {
      assert(start == final_offset);
      return;
    }

    for(unsigned i = 0; i < AS; ++i) {
      auto &e = nodes[start].edges[i];
      if(e) delete_subtree(e);
      e = 0;
    }
    // Mark node as empty and prepend to list of empty nodes
    nodes[start]          = EMPTY;
    nodes[start].edges[0] = empty_head;
    empty_head            = start;
  }

  // Pop the first empty node from the empty list, if any. Otherwise,
  // append a new one at the end of the nodes vector.
  OFFSET next_empty_node() {
    if(empty_head) {
      OFFSET res = empty_head;
      empty_head = nodes[res].edges[0];
      nodes[res].clear();
      nodes[res].state = INTERNAL;
      return res;
    }
    nodes.emplace_back(INTERNAL);
    return nodes.size() - 1;
  }

  // Recursively check equality between the nodes n1 and n2 in the ftries f1 and f2
  static bool equal_nodes(const ftrie& f1, const ftrie& f2, const node n1, const node n2) {
    for(unsigned i = 0; i < AS; ++i) {
      if(!n1.edges[i] != !n2.edges[i]) return false;
      if(n1.edges[i] != 0 && !equal_nodes(f1, f2, f1.nodes[n1.edges[i]], f2.nodes[n2.edges[i]])) return false;
    }
    return true;
  }

public:
  bool operator==(const ftrie& rhs) const {
    return equal_nodes(*this, rhs, nodes[0], rhs.nodes[0]);
  }

  // Insert a new string length size in the ftrie
  void insert(const char* s, size_t size) {
    if(size == 0) return;
    OFFSET cur = 0;

    if(size > 1) {
      for(size_t i = 0; i < size - 1; ++i) {
        assert(nodes[cur].state != EMPTY); // BUG!
        if(nodes[cur].state == FINAL) return; // Entire subtree is already assumed present
        const auto v = translate[s[i]];
        if(v < 0) throw std::range_error("Invalid letter in alphabet");
        const auto next = nodes[cur].edges[v];
        if(!next) {
          const auto ncur = next_empty_node();
          cur             = nodes[cur].edges[v] = ncur;
        } else {
          cur = next;
        }
      }
    }

    // Last letter
    const size_t i = size - 1;
    assert(nodes[cur].state != EMPTY); // BUG!
    if(nodes[cur].state == FINAL) return; // Entire subtree is already assumed present
    const auto v = translate[s[i]];
    if(v < 0) throw std::range_error("Invalid letter in alphabet");
    auto& next = nodes[cur].edges[v];
    if(next)
      delete_subtree(next);
    next = final_offset;
  }

  void insert(const char* s) { insert(s, strlen(s)); }
  void insert(const std::string& s) { insert(s.c_str(), s.size()); }

  FOUND find(const char* s, size_t size) const {
    const node* const vec = nodes.data();
    const node*       cur = vec;

    for(size_t i = 0; i < size; ++i) {
      assert(cur->state != EMPTY); // BUG!
      if(cur->state == FINAL) return HAS_PREFIX;
      const auto v = translate[s[i]];
      if(v < 0) throw std::range_error("Invalid letter in alphabet");
      const node* next = vec + cur->edges[v];
      if(next == vec) return NOT;
      cur = next;
    }

    assert(cur->state != EMPTY);
    return cur->state == FINAL ? HAS_PREFIX : PREFIX_OF;
  }

  FOUND find(const char* s) const { return find(s, strlen(s)); }
  FOUND find(const std::string& s) const { return find(s.c_str(), s.size()); }

  std::ostream& dump(std::ostream& os) const {
    os << alphabet << '\n'
       << empty_head << '\n';
    for(const auto& n : nodes) {
      os << n.state;
      for(unsigned i = 0; i < AS; ++i)
        os << ' ' << n.edges[i];
      os << '\n';
    }

    return os;
  }

  template<typename Container>
  void all_mers(unsigned depth, Container& mers, OFFSET start, std::string& mer) const {
    if(depth == 0) {
      mers.push_back(mer);
      return;
    }
    if(nodes[start].state == FINAL) {
      for(unsigned i = 0; i < AS; ++i) {
        mer[mer.size() - depth] = alphabet[i];
        all_mers(depth-1, mers, start, mer);
      }
      return;
    }
    for(unsigned i = 0; i < AS; ++i) {
      const auto next = nodes[start].edges[i];
      if(next) {
        mer[mer.size() - depth] = alphabet[i];
        all_mers(depth-1, mers, next, mer);
      }
    }
  }

  // Write all mers into any container that has a push_back(std::string) method.
  template<typename Container>
  auto all_mers(unsigned depth, Container& mers) const
    -> typename std::enable_if<std::is_same<void, decltype(mers.push_back(std::string()))>::value, void>::type
  {
    std::string mer(depth, '\0');
    all_mers(depth, mers, 0, mer);
  }

  // Create a vector with all mers
  std::vector<std::string> all_mers(unsigned depth) const {
    std::vector<std::string> res;
    all_mers(depth, res);
    return res;
  }

  // Write all mers, one per line, into an ostream
  std::ostream& all_mers(unsigned depth, std::ostream& os) const {
    struct StreamContainer {
      std::ostream& os;
      void push_back(const std::string& s) { os << s << '\n'; }
    };
    StreamContainer sc{os};
    all_mers(depth, sc);
    return os;
  }

  // Size of the encoded set of mers at a given depth. It is equal to
  // all_mers(depth).size(), but much faster.
  double size(unsigned depth, OFFSET start = root_offset) {
    if(depth == 0 || nodes[start].state == FINAL)
      return std::pow(alphabet.size(), depth);
    double res = 0;
    for(unsigned i = 0; i < AS; ++i) {
      const auto next = nodes[start].edges[i];
      if(next)
        res += size(depth - 1, next);
    }
    return res;
  }
};



#endif /* __FTRIE_H__ */
