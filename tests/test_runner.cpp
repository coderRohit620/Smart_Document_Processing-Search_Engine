/**
 * @file tests/test_runner.cpp
 * @brief Lightweight assertion-based test suite.
 *
 * No external testing framework required. Each test uses ASSERT_TRUE/ASSERT_EQ.
 *
 * Build & run:
 *   cmake --build build
 *   build\test_runner.exe        (Windows MinGW)
 *   ctest --test-dir build       (via CTest)
 */

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <fstream>

#include "TextProcessor.hpp"
#include "Document.hpp"
#include "DocumentManager.hpp"
#include "InvertedIndex.hpp"
#include "SearchEngine.hpp"
#include "LRUCache.hpp"

// ── Simple test framework ─────────────────────────────────────────────────

static int g_passed = 0;
static int g_failed = 0;

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            std::cerr << "  [FAIL] " << (msg) << "\n"; \
            ++g_failed; \
        } else { \
            std::cout << "  [PASS] " << (msg) << "\n"; \
            ++g_passed; \
        } \
    } while (false)

#define ASSERT_FALSE(condition, msg) ASSERT_TRUE(!(condition), msg)
#define ASSERT_EQ(a, b, msg)        ASSERT_TRUE((a) == (b), msg)

// ── Helper: create a temporary .txt file ─────────────────────────────────

static std::filesystem::path makeTempFile(const std::string& filename,
                                          const std::string& content)
{
    auto path = std::filesystem::temp_directory_path() / filename;
    std::ofstream ofs(path);
    ofs << content;
    return path;
}

// ─────────────────────────────────────────────────────────────────────────
// TextProcessor Tests
// ─────────────────────────────────────────────────────────────────────────

void test_tokenize_basic() {
    std::cout << "\n-- TextProcessor Tests --\n";
    auto tokens = TextProcessor::tokenize("Hello World", false);
    ASSERT_EQ(tokens.size(), 2u, "Two tokens from 'Hello World'");
    ASSERT_EQ(tokens[0], "hello", "First token lowercased");
    ASSERT_EQ(tokens[1], "world", "Second token lowercased");
}

void test_tokenize_cpp_special_case() {
    auto tokens = TextProcessor::tokenize("C++ programming", false);
    bool hasCpp = false;
    for (const auto& t : tokens) {
        if (t == "cpp") hasCpp = true;
    }
    ASSERT_TRUE(hasCpp, "'C++' normalizes to 'cpp'");
}

void test_tokenize_removes_punctuation() {
    auto tokens = TextProcessor::tokenize("memory, management!", false);
    ASSERT_EQ(tokens.size(), 2u, "Punctuation stripped, 2 tokens remain");
    ASSERT_EQ(tokens[0], "memory",     "First token correct");
    ASSERT_EQ(tokens[1], "management", "Second token correct");
}

void test_tokenize_stop_words() {
    // "the", "is" → stop-words; "algorithm", "fast" should survive.
    auto tokens = TextProcessor::tokenize("the algorithm is fast", true);
    ASSERT_EQ(tokens.size(), 2u, "Stop-words removed; 2 content tokens remain");
}

void test_tokenize_empty_string() {
    auto tokens = TextProcessor::tokenize("", true);
    ASSERT_TRUE(tokens.empty(), "Empty string produces zero tokens");
}

void test_tokenize_only_punctuation() {
    auto tokens = TextProcessor::tokenize("!!! ,,,", false);
    ASSERT_TRUE(tokens.empty(), "Only punctuation produces zero tokens");
}

void test_normalize_single_word() {
    ASSERT_EQ(TextProcessor::normalize("MEMORY"), "memory", "normalize uppercased word");
    ASSERT_EQ(TextProcessor::normalize("C++"),    "cpp",    "normalize C++");
}

void test_is_stopword() {
    ASSERT_TRUE( TextProcessor::isStopWord("the"),      "'the' is a stop-word");
    ASSERT_FALSE(TextProcessor::isStopWord("algorithm"),"'algorithm' is not a stop-word");
}

// ─────────────────────────────────────────────────────────────────────────
// Document Tests
// ─────────────────────────────────────────────────────────────────────────

void test_document_creation() {
    std::cout << "\n-- Document Tests --\n";
    Document doc(1, "test.txt", "/data/test.txt", "Hello world test");
    ASSERT_EQ(doc.getID(),     1,          "Document ID correct");
    ASSERT_EQ(doc.getTitle(),  "test.txt", "Title correct");
    ASSERT_EQ(doc.wordCount(), 3u,         "Word count correct");
}

// ─────────────────────────────────────────────────────────────────────────
// InvertedIndex Tests
// ─────────────────────────────────────────────────────────────────────────

void test_index_add_and_search() {
    std::cout << "\n-- InvertedIndex Tests --\n";
    InvertedIndex idx;
    Document doc1(1, "d1.txt", "/d1.txt", "C++ memory management");
    Document doc2(2, "d2.txt", "/d2.txt", "memory leaks dangerous");

    idx.addDocument(doc1);
    idx.addDocument(doc2);

    auto result = idx.search("memory");
    ASSERT_TRUE(result.size() == 2u, "Both docs contain 'memory'");

    auto result2 = idx.search("cpp");
    ASSERT_EQ(result2.size(), 1u, "Only doc1 contains 'cpp'");
    ASSERT_EQ(result2[0], 1,      "Doc1 ID returned for 'cpp'");
}

void test_index_term_frequency() {
    InvertedIndex idx;
    Document doc(1, "d.txt", "/d.txt", "C++ is great. C++ is fast. I love C++.");
    idx.addDocument(doc);

    int tf = idx.getTermFrequency("cpp", 1);
    ASSERT_EQ(tf, 3, "Term frequency of 'cpp' in doc1 is 3");
}

void test_index_remove_document() {
    InvertedIndex idx;
    Document doc1(1, "d1.txt", "/d1.txt", "C++ memory");
    Document doc2(2, "d2.txt", "/d2.txt", "Python memory");
    idx.addDocument(doc1);
    idx.addDocument(doc2);

    idx.removeDocument(1);

    auto result = idx.search("cpp");
    ASSERT_TRUE(result.empty(), "After removal, 'cpp' returns no results");

    auto memResult = idx.search("memory");
    ASSERT_EQ(memResult.size(), 1u, "Doc2 still indexed for 'memory'");
    ASSERT_EQ(memResult[0], 2,      "Remaining doc is doc2");
}

void test_index_search_missing_term() {
    InvertedIndex idx;
    Document doc(1, "d.txt", "/d.txt", "algorithms data structures");
    idx.addDocument(doc);

    auto result = idx.search("neuralnetwork");
    ASSERT_TRUE(result.empty(), "Missing term returns empty results");
}

void test_index_clear() {
    InvertedIndex idx;
    Document doc(1, "d.txt", "/d.txt", "test content here");
    idx.addDocument(doc);
    idx.clear();
    ASSERT_EQ(idx.termCount(), 0u, "Index cleared successfully");
}

// ─────────────────────────────────────────────────────────────────────────
// DocumentManager Tests
// ─────────────────────────────────────────────────────────────────────────

void test_document_manager_add() {
    std::cout << "\n-- DocumentManager Tests --\n";
    auto path = makeTempFile("mgr_test.txt", "smart pointers in cpp");

    DocumentManager mgr;
    try {
        const Document* doc = mgr.addDocument(path);
        ASSERT_TRUE(doc != nullptr,           "Document added successfully");
        ASSERT_EQ(mgr.documentCount(), 1u,    "One document in manager");
        ASSERT_TRUE(mgr.documentExists(doc->getID()), "Document found by ID");
    } catch (const std::exception& e) {
        std::cerr << "  Exception: " << e.what() << "\n";
        ASSERT_TRUE(false, "Exception thrown during addDocument");
    }

    std::filesystem::remove(path);
}

void test_document_manager_remove() {
    auto path = makeTempFile("mgr_remove_test.txt", "inverted index search engine");

    DocumentManager mgr;
    const Document* doc = mgr.addDocument(path);
    int id = doc->getID();

    bool removed = mgr.removeDocument(id);
    ASSERT_TRUE(removed,                 "removeDocument returns true");
    ASSERT_EQ(mgr.documentCount(), 0u,   "Manager now empty");
    ASSERT_FALSE(mgr.documentExists(id), "Document no longer exists");

    std::filesystem::remove(path);
}

void test_document_manager_duplicate_rejection() {
    auto path = makeTempFile("dup_test.txt", "duplicate file test content");

    DocumentManager mgr;
    mgr.addDocument(path);

    bool threw = false;
    try {
        mgr.addDocument(path); // Should throw – same title already loaded
    } catch (const std::runtime_error&) {
        threw = true;
    }
    ASSERT_TRUE(threw, "Duplicate document rejected with exception");

    std::filesystem::remove(path);
}

void test_document_manager_invalid_file() {
    DocumentManager mgr;
    bool threw = false;
    try {
        mgr.addDocument("nonexistent_file_xyz.txt");
    } catch (const std::exception&) {
        threw = true;
    }
    ASSERT_TRUE(threw, "Invalid file path throws exception");
}

// ─────────────────────────────────────────────────────────────────────────
// SearchEngine Tests
// ─────────────────────────────────────────────────────────────────────────

// Helper struct for building a local search environment.
struct TestEnv {
    DocumentManager docMgr;
    InvertedIndex   index;
    SearchEngine    engine{index, docMgr, 3};
    std::vector<std::filesystem::path> paths;

    void add(const std::string& fname, const std::string& content) {
        auto path = makeTempFile(fname, content);
        const Document* doc = docMgr.addDocument(path);
        index.addDocument(*doc);
        paths.push_back(path);
    }

    ~TestEnv() {
        for (auto& p : paths) std::filesystem::remove(p);
    }
};

void test_search_basic() {
    std::cout << "\n-- SearchEngine Tests --\n";
    TestEnv env;
    env.add("s1.txt", "C++ memory management smart pointers");
    env.add("s2.txt", "Python machine learning neural networks");

    bool hit = false;
    auto results = env.engine.search("cpp", hit);
    ASSERT_EQ(results.size(), 1u, "Only doc1 matches 'cpp'");
    ASSERT_FALSE(hit,             "First search is a cache miss");
}

void test_search_cache_hit() {
    TestEnv env;
    env.add("c1.txt", "C++ is a systems programming language");

    bool hit = false;
    env.engine.search("cpp", hit);   // First: miss
    ASSERT_FALSE(hit, "First call is cache miss");

    env.engine.search("cpp", hit);   // Second: hit
    ASSERT_TRUE(hit, "Second call is cache hit");
}

void test_search_ranking() {
    TestEnv env;
    env.add("r1.txt", "memory management and memory allocation in memory");
    env.add("r2.txt", "memory is important");

    bool hit = false;
    auto results = env.engine.search("memory", hit);
    ASSERT_TRUE(results.size() >= 2u,
                "At least 2 results for 'memory'");
    ASSERT_TRUE(results[0].score >= results[1].score,
                "Results sorted by score descending");
}

void test_search_multi_keyword() {
    TestEnv env;
    env.add("mk1.txt", "C++ memory management is powerful and efficient");
    env.add("mk2.txt", "Java memory management uses garbage collection");
    env.add("mk3.txt", "Python is slow for systems programming");

    bool hit = false;
    auto results = env.engine.search("memory management", hit);
    ASSERT_TRUE(results.size() >= 2u, "Multi-keyword returns multiple results");
}

void test_search_empty_query_throws() {
    TestEnv env;
    env.add("eq.txt", "some content here for searching");

    bool hit = false;
    bool threw = false;
    try {
        env.engine.search("", hit);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    ASSERT_TRUE(threw, "Empty query throws std::invalid_argument");
}

void test_search_no_results() {
    TestEnv env;
    env.add("nr.txt", "data structures algorithms graphs trees");

    bool hit = false;
    auto results = env.engine.search("neuralnetwork", hit);
    ASSERT_TRUE(results.empty(), "Non-matching term returns empty results");
}

// ─────────────────────────────────────────────────────────────────────────
// LRU Cache Tests
// ─────────────────────────────────────────────────────────────────────────

void test_lru_basic_put_get() {
    std::cout << "\n-- LRU Cache Tests --\n";
    LRUCache<std::string, int> cache(3);
    cache.put("a", 1);
    cache.put("b", 2);

    auto valA = cache.get("a");
    ASSERT_TRUE(valA.has_value(), "Key 'a' found in cache");
    ASSERT_EQ(valA.value(), 1,    "Value of 'a' is 1");
}

void test_lru_miss() {
    LRUCache<std::string, int> cache(3);
    auto val = cache.get("missing");
    ASSERT_FALSE(val.has_value(), "Missing key returns nullopt");
}

void test_lru_eviction() {
    // Insert a, b, c then d → 'a' evicted (LRU order: a→b→c, insert d: a gone)
    LRUCache<std::string, int> cache(3);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);
    cache.put("d", 4);  // 'a' should be evicted

    ASSERT_FALSE(cache.contains("a"), "LRU entry 'a' was evicted");
    ASSERT_TRUE( cache.contains("b"), "'b' still in cache");
    ASSERT_TRUE( cache.contains("c"), "'c' still in cache");
    ASSERT_TRUE( cache.contains("d"), "'d' newly inserted");
}

void test_lru_access_promotes_to_front() {
    // Insert a, b, c; access 'a' (promotes to MRU); then insert d.
    // 'b' becomes LRU after 'a' is promoted; 'b' should be evicted.
    LRUCache<std::string, int> cache(3);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("c", 3);

    cache.get("a");     // Promote 'a' to MRU; order: a, c, b → b is LRU

    cache.put("d", 4);  // 'b' evicted

    ASSERT_FALSE(cache.contains("b"), "'b' evicted as LRU after 'a' promoted");
    ASSERT_TRUE( cache.contains("a"), "'a' was recently accessed – kept");
    ASSERT_TRUE( cache.contains("c"), "'c' still in cache");
    ASSERT_TRUE( cache.contains("d"), "'d' newly inserted");
}

void test_lru_update_existing_key() {
    LRUCache<std::string, int> cache(3);
    cache.put("x", 10);
    cache.put("x", 99);  // Update value

    auto val = cache.get("x");
    ASSERT_TRUE(val.has_value(),  "Key 'x' still present after update");
    ASSERT_EQ(val.value(), 99,    "Value updated to 99");
    ASSERT_EQ(cache.size(), 1u,   "Size still 1 after update");
}

void test_lru_zero_capacity_throws() {
    bool threw = false;
    try {
        LRUCache<std::string, int> cache(0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    ASSERT_TRUE(threw, "Zero capacity throws std::invalid_argument");
}

void test_lru_cache_query_sequence() {
    // Simulate the spec example:
    //   capacity=3, queries: C++, memory, algorithms, C++
    //   Second "C++" → cache hit
    LRUCache<std::string, std::vector<int>> cache(3);
    cache.put("cpp",        {1, 2});
    cache.put("memory",     {1, 3});
    cache.put("algorithms", {4});

    auto hit = cache.get("cpp"); // Should be a cache hit
    ASSERT_TRUE(hit.has_value(), "C++ query is a cache hit on second access");
}

void test_lru_size_and_capacity() {
    LRUCache<int, int> cache(5);
    ASSERT_EQ(cache.capacity(), 5u, "Capacity is 5");
    ASSERT_EQ(cache.size(),     0u, "Initially empty");
    cache.put(1, 10);
    cache.put(2, 20);
    ASSERT_EQ(cache.size(), 2u, "Size 2 after two inserts");
}

// ═══════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "\n╔═══════════════════════════════════════════╗\n";
    std::cout <<   "║    Smart Document Search – Test Suite     ║\n";
    std::cout <<   "╚═══════════════════════════════════════════╝\n";

    // TextProcessor
    test_tokenize_basic();
    test_tokenize_cpp_special_case();
    test_tokenize_removes_punctuation();
    test_tokenize_stop_words();
    test_tokenize_empty_string();
    test_tokenize_only_punctuation();
    test_normalize_single_word();
    test_is_stopword();

    // Document
    test_document_creation();

    // InvertedIndex
    test_index_add_and_search();
    test_index_term_frequency();
    test_index_remove_document();
    test_index_search_missing_term();
    test_index_clear();

    // DocumentManager
    test_document_manager_add();
    test_document_manager_remove();
    test_document_manager_duplicate_rejection();
    test_document_manager_invalid_file();

    // SearchEngine
    test_search_basic();
    test_search_cache_hit();
    test_search_ranking();
    test_search_multi_keyword();
    test_search_empty_query_throws();
    test_search_no_results();

    // LRU Cache
    test_lru_basic_put_get();
    test_lru_miss();
    test_lru_eviction();
    test_lru_access_promotes_to_front();
    test_lru_update_existing_key();
    test_lru_zero_capacity_throws();
    test_lru_cache_query_sequence();
    test_lru_size_and_capacity();

    std::cout << "\n═══════════════════════════════════════════\n";
    std::cout << "  Results: " << g_passed << " passed, "
                               << g_failed << " failed.\n";
    std::cout << "═══════════════════════════════════════════\n\n";

    return (g_failed == 0) ? 0 : 1;
}
