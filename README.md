# Description

The code in this repository is a complement to the paper [Practical universal _k_-mer sets for minimizer schemes](https://dl.acm.org/citation.cfm?id=3342144).
Its provide a C++ library to read and query the _k_-mer data files available on Zonodo ([All General and Human sK2-sK6](https://zenodo.org/record/3385067), [Human sK7-sK10](https://zenodo.org/record/3385069)).

# Usage

To install, simply copy the file `ftrie.hpp` into your own project.

The following code reads a file, and queries whether the _k_-mer `ACGT` is present in the trie.

```C++
#include <iostream>
#include <fstream>
#include "ftrie.hpp"

typedef ftrie<4> trie_type; // Trie with alphabet of size 4

int main(int argc, char *argv[]) {
  std::ifstream ftrie_in("path/to/file");
  trie_type uhmers(ftrie_in);
  ftrie_in.close();

  // Search for ACGT
  switch(uhmers.find("ACGT")) {
  case trie_type::NOT:
    std::cerr << "Not found\n";
    break;

  case trie_type::HAS_PREFIX:
    std::cerr << "A prefix of ACGT is present in the trie\n";
    break;

  case trie_type::PREFIX_OF:
    std::cerr << "ACGT is a prefix of a k-mer in the trie\n";
    break;
  }

  return 0;
}
```
