/**
 * @file SearchEngine.cpp
 * @brief Implements query processing, ranking, and cache management.
 *
 * Ranking algorithm (TF-based scoring):
 *   For each candidate document, the score is the sum of the term-frequency
 *   of each query term within that document.
 *
 *   Score(doc, query) = Σ tf(term_i, doc) for each term_i in query
 *
 *   Example:
 *     query = "memory cpp"
 *     doc1 contains "memory" 2×, "cpp" 3×  → score = 5
 *     doc2 contains "memory" 1×, "cpp" 1×  → score = 2
 *     → doc1 ranks higher
 *
 *   Documents not containing ANY query term are excluded entirely.
 *
 *   For a multi-keyword query, candidate documents are the UNION of all
 *   docID sets returned per term (OR semantics). A document that matches
 *   more terms scores higher naturally, because its tf sum is larger.
 */

#include "SearchEngine.hpp"
#include "Index.hpp"
#include "InvertedIndex.hpp"
#include "DocumentManager.hpp"
#include "TextProcessor.hpp"
#include "Document.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>

// ── Constructor ────────────────────────────────────────────────────────────

SearchEngine::SearchEngine(Index& index,
                           const DocumentManager& docMgr,
                           std::size_t cacheCapacity)
    : m_index(index)
    , m_docMgr(docMgr)
    , m_cache(cacheCapacity)
{}

// ── Search ─────────────────────────────────────────────────────────────────

std::vector<SearchResult>
SearchEngine::search(const std::string& query, bool& cacheHit)
{
    if (query.empty()) {
        throw std::invalid_argument("Search query cannot be empty.");
    }

    // Step 1: Normalize the query into tokens.
    // We use removeStops=false so short, meaningful terms like "a*" survive.
    // Actually for searching we do want stop-words removed for cleaner results.
    std::vector<std::string> terms =
        TextProcessor::tokenize(query, /*removeStops=*/true);

    if (terms.empty()) {
        throw std::invalid_argument(
            "Query contains only stop-words or non-indexable characters.");
    }

    // Build a canonical cache key: sorted, space-joined terms.
    // Sorting ensures "cpp memory" and "memory cpp" hit the same cache entry.
    std::vector<std::string> sortedTerms = terms;
    std::sort(sortedTerms.begin(), sortedTerms.end());
    std::string cacheKey;
    for (const auto& t : sortedTerms) {
        if (!cacheKey.empty()) cacheKey += ' ';
        cacheKey += t;
    }

    // Step 2: Check LRU cache.
    auto cached = m_cache.get(cacheKey);
    if (cached.has_value()) {
        cacheHit = true;
        return cached.value();
    }
    cacheHit = false;

    // Step 3: Gather candidate document IDs (union across all query terms).
    std::unordered_set<int> candidateIDs;
    for (const std::string& term : terms) {
        std::vector<int> matchingDocs = m_index.search(term);
        for (int id : matchingDocs) {
            candidateIDs.insert(id);
        }
    }

    // Step 4: Score each candidate.
    std::vector<SearchResult> results;
    results.reserve(candidateIDs.size());

    for (int docID : candidateIDs) {
        const Document* doc = m_docMgr.getDocument(docID);
        if (!doc) continue;  // Document may have been removed after indexing.

        int score = computeScore(docID, terms);
        if (score > 0) {
            results.push_back({docID, doc->getTitle(), score});
        }
    }

    // Step 5: Sort by score descending using std::sort + lambda comparator.
    std::sort(results.begin(), results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.score > b.score;  // Higher score → earlier in list
              });

    // Step 6: Store in cache for future queries.
    m_cache.put(cacheKey, results);

    return results;
}

// ── Cache Inspection ───────────────────────────────────────────────────────

void SearchEngine::printCacheInfo() const {
    std::cout << "\n=== LRU Cache Information ===\n";
    std::cout << "  Capacity : " << m_cache.capacity() << '\n';
    std::cout << "  Size     : " << m_cache.size()     << '\n';

    auto keys = m_cache.keysInOrder();
    if (keys.empty()) {
        std::cout << "  Cache is empty.\n";
    } else {
        std::cout << "  Cached queries (MRU → LRU):\n";
        for (std::size_t i = 0; i < keys.size(); ++i) {
            std::cout << "    " << (i + 1) << ". \"" << keys[i] << "\"\n";
        }
    }
    std::cout << '\n';
}

std::size_t SearchEngine::cacheSize()     const { return m_cache.size(); }
std::size_t SearchEngine::cacheCapacity() const { return m_cache.capacity(); }

bool SearchEngine::isCached(const std::string& query) const {
    // Build canonical key the same way as search().
    std::vector<std::string> terms =
        TextProcessor::tokenize(query, /*removeStops=*/true);
    std::sort(terms.begin(), terms.end());
    std::string cacheKey;
    for (const auto& t : terms) {
        if (!cacheKey.empty()) cacheKey += ' ';
        cacheKey += t;
    }
    return m_cache.contains(cacheKey);
}

void SearchEngine::clearCache() {
    m_cache.clear();
}

// ── Private Helpers ────────────────────────────────────────────────────────

int SearchEngine::computeScore(int docID,
                               const std::vector<std::string>& terms) const
{
    // Cast to InvertedIndex to access getTermFrequency.
    // This is safe because we know the index is always an InvertedIndex.
    // In a larger system this would be part of the Index interface.
    const auto* invertedIdx = dynamic_cast<const InvertedIndex*>(&m_index);
    if (!invertedIdx) return 0;

    int score = 0;
    for (const std::string& term : terms) {
        score += invertedIdx->getTermFrequency(term, docID);
    }
    return score;
}
