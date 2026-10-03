/**
 * @file SearchEngine.hpp
 * @brief Ties together the index, document manager, and LRU cache to
 *        process and rank search queries.
 *
 * Search pipeline (one query):
 *   1. Normalize + tokenize the query string.
 *   2. Check the LRU cache.  On HIT → return cached results immediately.
 *   3. For each query term, look up the inverted index → set of docIDs.
 *   4. Compute a relevance score for each candidate document.
 *   5. Sort results by score (descending).
 *   6. Store results in cache.
 *   7. Return ranked results.
 *
 * ── Ranking Algorithm ──────────────────────────────────────────────────────
 *   Score(doc, query) = Σ  tf(term, doc)  for each query term present in doc
 *
 *   tf(term, doc) = number of times the term appears in the document.
 *
 *   This is a simplified TF (term-frequency) model.  It is intentionally
 *   kept simple so it can be explained in 30 seconds at an interview.
 *
 *   Limitation: longer documents naturally accumulate higher raw TF.
 *   A production system would normalise by document length (TF-IDF + BM25),
 *   but that complexity is unnecessary for a fresher portfolio project.
 *
 * OOP: SearchEngine holds a reference to the Index via its abstract type,
 *      demonstrating polymorphism – it works with any Index subclass.
 */

#pragma once

#include "LRUCache.hpp"
#include <string>
#include <vector>
#include <memory>

class Index;
class DocumentManager;

// ── Result type ───────────────────────────────────────────────────────────

/**
 * @brief Represents a single search result.
 */
struct SearchResult {
    int         docID;    ///< Document identifier
    std::string title;    ///< File name / display name
    int         score;    ///< Relevance score (higher is better)

    // Enables sorting in descending order of score.
    bool operator>(const SearchResult& other) const {
        return score > other.score;
    }
};

// ── SearchEngine ──────────────────────────────────────────────────────────

class SearchEngine {
public:
    /**
     * @brief Construct the search engine.
     *
     * @param index    Reference to any Index implementation (polymorphism).
     * @param docMgr   Reference to the document manager.
     * @param cacheCapacity  Number of recent queries to cache.
     */
    SearchEngine(Index& index,
                 const DocumentManager& docMgr,
                 std::size_t cacheCapacity = 10);

    // ── Search ─────────────────────────────────────────────────────────────

    /**
     * @brief Execute a search query and return ranked results.
     *
     * @param query     Raw user query string (e.g., "C++ memory").
     * @param cacheHit  Output parameter: set to true if result came from cache.
     * @return          Vector of SearchResult sorted by score descending.
     * @throws std::invalid_argument if the query is empty after normalization.
     */
    std::vector<SearchResult> search(const std::string& query,
                                     bool& cacheHit);

    // ── Cache Inspection ───────────────────────────────────────────────────
    void        printCacheInfo()                          const;
    std::size_t cacheSize()                               const;
    std::size_t cacheCapacity()                           const;
    bool        isCached(const std::string& query)        const;
    void        clearCache();

private:
    /**
     * @brief Compute a relevance score for a document given a set of terms.
     *        Score = sum of term-frequencies across all query terms.
     */
    int computeScore(int docID,
                     const std::vector<std::string>& terms) const;

    // ── Members ────────────────────────────────────────────────────────────
    Index&                 m_index;   ///< Abstract index (polymorphism)
    const DocumentManager& m_docMgr;

    // LRU cache: normalized-query-string → ranked results
    LRUCache<std::string, std::vector<SearchResult>> m_cache;
};
