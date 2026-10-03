# Smart Document Search — Interview Prep Guide

## Final Project Structure

```
SmartDocumentSearch/
├── include/
│   ├── Document.hpp        — Encapsulated model (Encapsulation)
│   ├── Index.hpp           — Abstract interface (Abstraction + Polymorphism)
│   ├── InvertedIndex.hpp   — Concrete index (Inheritance + Polymorphism)
│   ├── DocumentManager.hpp — CRUD + unique_ptr ownership
│   ├── TextProcessor.hpp   — Stateless tokenizer utility
│   ├── FileProcessor.hpp   — RAII file reader
│   ├── SearchEngine.hpp    — Query pipeline + LRU cache integration
│   └── LRUCache.hpp        — Generic LRU (list + unordered_map)
├── src/                    — Implementations for all above
├── data/                   — 5 sample .txt documents
├── tests/test_runner.cpp   — 62 assertions, no external framework
├── CMakeLists.txt
└── README.md
```

---

## Build & Run

```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel 4
build\SmartDocSearch.exe      # Run app
build\test_runner.exe         # Run tests  →  62/62 pass
```

---

## Module-by-Module Explanation

### 1. Document
**File:** [Document.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/Document.hpp) | [Document.cpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/src/Document.cpp)

Represents a single `.txt` file loaded into the system.

- All fields (`m_id`, `m_title`, `m_path`, `m_content`) are `private` → **Encapsulation**
- Public getters return `const std::string&` → zero-copy read-only access
- Constructor uses `std::move` → transfers string buffer O(1) instead of O(n) copy
- **Rule of 0**: no custom destructor/copy/move — STL types manage themselves

**Interview:** "Why `const std::string&` in getters?" → Returns a reference — O(1), no copy, caller cannot mutate.

---

### 2. Index (Abstract Interface)
**File:** [Index.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/Index.hpp)

Abstract base class defining the index contract.

- All methods are `pure virtual` (`= 0`) → **Abstraction**: callers know WHAT, not HOW
- `virtual ~Index() = default` → mandatory for safe polymorphic destruction
- `SearchEngine` holds `Index&` → **Polymorphism**: works with any concrete index

**Interview:** "Why virtual destructor?" → Without it, deleting a derived object via a base pointer only calls the base destructor, leaking the derived part.

---

### 3. InvertedIndex
**File:** [InvertedIndex.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/InvertedIndex.hpp) | [InvertedIndex.cpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/src/InvertedIndex.cpp)

Concrete index backed by nested hash maps. Demonstrates **Inheritance** (`: public Index`) and **Polymorphism** (`override`).

Core structure:
```
unordered_map<string, unordered_map<int, int>>
      term  →         docID → term-frequency
```

- `unordered_map` → O(1) average lookup (vs O(log N) for `map`)
- `m_docTokens` stores each doc's token list for O(T) removal

**Interview:** "How do you remove a document?" → Look up its tokens in `m_docTokens`, erase its docID from each term's postings. O(T) where T = unique tokens.

---

### 4. TextProcessor
**File:** [TextProcessor.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/TextProcessor.hpp) | [TextProcessor.cpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/src/TextProcessor.cpp)

Stateless normalizer/tokenizer. Pipeline: C++→cpp → lowercase → strip punctuation → split → drop empty → optional stop-word removal.

- All methods `static` — pure utility, no state
- Stop-word set: function-local static (C++11 thread-safe, constructed once)

**Interview:** "Why convert C++ to cpp?" → '+' is not alphanumeric and would be stripped. Mapping to 'p' preserves the term.

---

### 5. FileProcessor
**File:** [FileProcessor.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/FileProcessor.hpp) | [FileProcessor.cpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/src/FileProcessor.cpp)

RAII file reader. `std::ifstream` constructor opens the file; destructor closes it automatically — even if an exception is thrown.

**Interview:** "What is RAII?" → Resource Acquisition Is Initialization. Resource lifetime = object lifetime. Destructor always runs, even on exception (stack unwinding). No finally{} needed.

---

### 6. DocumentManager
**File:** [DocumentManager.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/DocumentManager.hpp) | [DocumentManager.cpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/src/DocumentManager.cpp)

CRUD for documents. Stores `unordered_map<int, unique_ptr<Document>>` — sole ownership, automatic cleanup.

- `make_unique<Document>(...)` — no raw `new`, no manual `delete`
- `m_documents.erase(id)` → `unique_ptr` destructor frees the Document

**Interview:** "Why `unique_ptr` not `shared_ptr`?" → Sole ownership here. `shared_ptr` adds reference-count overhead with zero benefit.

---

### 7. SearchEngine
**File:** [SearchEngine.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/SearchEngine.hpp) | [SearchEngine.cpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/src/SearchEngine.cpp)

Query pipeline: normalize → cache check → index lookup → score → sort → cache store.

- Holds `Index&` → polymorphism (any Index subclass works)
- Cache key = sorted terms joined by space (query order-independent)
- `std::sort` with lambda for descending score

---

### 8. LRUCache (Template)
**File:** [LRUCache.hpp](file:///e:/Project_2026/C++_Project/Smart_Document_Processing-Search_Engine/include/LRUCache.hpp)

Generic `LRUCache<Key, Value>` using:
- `std::list` (doubly-linked) — MRU at front, LRU at back
- `std::unordered_map<Key, ListIter>` — O(1) jump to list node

get: map lookup → splice to front → O(1) avg  
put: if exists update+splice; if new, evict back if full, push front → O(1) avg

---

## OOP, DSA, STL, RAII — Where Each Lives

### OOP

| Concept | File | How |
|---|---|---|
| **Encapsulation** | Document, DocumentManager | Private fields, public const getters |
| **Abstraction** | Index.hpp | Pure virtual interface |
| **Inheritance** | InvertedIndex → Index | `: public Index` |
| **Polymorphism** | SearchEngine `Index&` | Virtual dispatch at runtime |

### DSA

| Concept | File |
|---|---|
| Inverted index | InvertedIndex |
| Hash map | InvertedIndex, DocumentManager, LRUCache |
| Doubly-linked list | LRUCache |
| Sorting | SearchEngine (`std::sort`) |
| Tokenization | TextProcessor |
| TF scoring | SearchEngine::computeScore |

### Smart Pointers & RAII

| Feature | Where |
|---|---|
| `unique_ptr` ownership | DocumentManager |
| `make_unique` | DocumentManager::createDocument |
| RAII file | FileProcessor (`std::ifstream`) |
| Move semantics | Document constructor |
| Rule of 0 | All classes |

### STL

| Container/Algo | Where |
|---|---|
| `unordered_map` | InvertedIndex, DocumentManager, LRUCache |
| `unordered_set` | InvertedIndex::removeDocument, stop-words |
| `list` | LRUCache |
| `vector` | Results, token lists |
| `optional` | LRUCache::get return |
| `filesystem` | FileProcessor, DocumentManager |
| `sort` / `partial_sort` | SearchEngine, InvertedIndex::printStats |
| `transform` | TextProcessor (lowercase) |

---

## Complexity Table

| Operation | Average | Worst |
|---|---|---|
| Add document to index | O(L) | O(L) |
| Single keyword lookup | O(D) | O(D+n) |
| Multi-keyword search (K terms) | O(K·D + N log N) | O(K·n + N log N) |
| Sort results | O(N log N) | O(N log N) |
| Remove document | O(T) | O(T) |
| LRU cache get | **O(1) avg** | O(n) |
| LRU cache put | **O(1) avg** | O(n) |

L=chars, D=docs/term, N=result count, T=unique tokens, n=cache size

---

## 15 Interview Questions — Model Answers

**Q1. Walk me through your project.**
> "It's a command-line search engine. I load .txt files, tokenize content with TextProcessor, and build an inverted index — a hash map from word → documents containing it. When a user searches, I look up each keyword in the index, score each matching document by summing term frequencies, sort by score, and return ranked results. I also cache recent queries in a custom LRU cache."

**Q2. What is an inverted index and why did you use it?**
> "An inverted index maps word → set of documents, like the index at the back of a textbook. Without it, every search scans every document — O(N×L). With it, a keyword lookup is O(1) average — just a hash map access."

**Q3. Why `unordered_map` instead of `map`?**
> "`std::map` gives O(log N) and maintains sorted order. `unordered_map` gives O(1) average. For a search engine, lookup speed beats ordering."

**Q4. How does your LRU cache work?**
> "I combine a doubly-linked list and a hash map. The list tracks recency — MRU at front, LRU at back. The hash map maps each key to its node's iterator for O(1) access. On hit, splice node to front (O(1)). On miss + full capacity, pop_back (evict LRU) and push_front (new entry)."

**Q5. Why O(1) *average* for LRU, not O(1) guaranteed?**
> "unordered_map uses a hash function. Average is O(1), but worst case with all keys in one bucket is O(n). With good hash functions this is rare, but technically it's average O(1)."

**Q6. Explain the four OOP concepts in your project.**
> "Encapsulation: Document's fields are private, accessed via const getters. Abstraction: Index is pure virtual — callers know what operations exist, not how. Inheritance: InvertedIndex extends Index. Polymorphism: SearchEngine holds an `Index&` reference and calls virtual methods dispatched at runtime."

**Q7. What is RAII? Where do you use it?**
> "RAII ties resource lifetime to object lifetime. FileProcessor uses `std::ifstream` — file opened in constructor, auto-closed in destructor — even on exception. DocumentManager uses `unique_ptr` — Document freed when erased from the map."

**Q8. Why `unique_ptr` not `shared_ptr`?**
> "DocumentManager is the sole owner. `shared_ptr` adds reference-count overhead with zero benefit here. `unique_ptr` communicates sole ownership and is zero-cost."

**Q9. What is a memory leak? What causes a dangling pointer?**
> "Memory leak: `new` without `delete` — memory consumed but never returned. Dangling pointer: `delete` a pointer then dereference it — undefined behavior. Smart pointers prevent both."

**Q10. Explain copy vs move semantics.**
> "Copy duplicates data — O(n) for a string. Move transfers the internal buffer in O(1); the source is valid but empty. Document's constructor uses `std::move` on its string arguments."

**Q11. Rule of 0, Rule of 3, Rule of 5?**
> "Rule of 0: don't define destructor/copy/move if member types manage themselves — my classes use STL so compiler-generated is correct. Rule of 3: define destructor → also define copy ctor + copy assignment. Rule of 5: + move ctor + move assignment."

**Q12. Time complexity of a 3-keyword search?**
> "Lookup 3 terms: O(3×D). Gather candidate union: O(3×D). Score N candidates: O(N×3). Sort: O(N log N). Total: O(K×D + N×K + N log N)."

**Q13. How does `std::sort` work?**
> "Introsort — a hybrid of quicksort (avg O(n log n)), heapsort (worst-case guarantee O(n log n)), and insertion sort (fast on small n). Guarantees O(n log n) worst case."

**Q14. `map` vs `unordered_map` in all dimensions?**
> "map: BST, O(log N) ops, keys sorted, no hash needed. unordered_map: hash table, O(1) avg ops, keys unordered, requires hashable key, O(n) worst case."

**Q15. If you had more time, what would you improve?**
> "TF-IDF to penalize common cross-document terms. BM25 for production-quality ranking. Phrase search via positional postings. Persistent index serialization. Parallel indexing with std::thread."
