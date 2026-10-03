/**
 * @file InvertedIndex.hpp
 * @brief Concrete inverted index backed by hash maps.
 *
 * OOP Concepts:
 *   INHERITANCE  – InvertedIndex extends Index.
 *   POLYMORPHISM – All Index pure-virtual methods are overridden here.
 *
 * ── What is an Inverted Index? ─────────────────────────────────────────────
 *   A forward index maps:  document → list of words it contains.
 *   An inverted index maps: word → set of documents that contain the word.
 *
 *   Analogy: the index at the back of a textbook maps keyword → page numbers.
 *
 *   This allows keyword search in O(1) average per term rather than O(N*L)
 *   for a naive linear scan over N documents of average length L.
 *
 * ── Internal Data Structures ───────────────────────────────────────────────
 *
 *   m_index:
 *     unordered_map<string, unordered_map<int, int>>
 *       │                       │             │
 *       term                 docID         term-frequency (tf)
 *
 *     Example after indexing three docs:
 *       "cpp"    → {1:3, 2:1}    (doc 1 has "cpp" 3 times, doc 2 once)
 *       "memory" → {1:2, 2:4}
 *
 *   m_docTokens:
 *     unordered_map<int, vector<string>>
 *     Stores the token list for each document so we can rebuild the index
 *     or remove a document without re-reading the file.
 *
 * ── Why unordered_map? ─────────────────────────────────────────────────────
 *   - O(1) average insert and lookup (vs O(log N) for std::map).
 *   - For a search engine, lookup speed is more important than key ordering.
 *   - std::map would give O(log N) but would maintain alphabetical order –
 *     that ordering is unnecessary here.
 */

#pragma once

#include "Index.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Document; // Forward declaration

class InvertedIndex : public Index {
public:
    InvertedIndex() = default;

    // ── Index interface ────────────────────────────────────────────────────
    void addDocument(const Document& doc)              override;
    void removeDocument(int docID)                     override;
    std::vector<int> search(const std::string& term)   const override;
    void rebuild()                                     override;
    void clear()                                       override;
    std::size_t termCount()                            const override;

    // ── Extended API ───────────────────────────────────────────────────────

    /**
     * @brief Return the term-frequency of a term in a specific document.
     *        Returns 0 if the term does not appear in that document.
     *
     * Term frequency (tf) = number of times the term appears in the document.
     * Used by SearchEngine to compute relevance scores.
     */
    int getTermFrequency(const std::string& term, int docID) const;

    /**
     * @brief Return the number of documents that contain the given term.
     *        Used to compute IDF-like weighting in ranking.
     */
    std::size_t documentFrequency(const std::string& term) const;

    /**
     * @brief Print index statistics to stdout.
     */
    void printStats() const;

    /**
     * @brief Return all unique terms in the index (for display/debugging).
     */
    std::vector<std::string> allTerms() const;

private:
    // word → { docID → term-frequency }
    std::unordered_map<std::string,
                       std::unordered_map<int, int>> m_index;

    // docID → token list  (needed to correctly remove a document)
    std::unordered_map<int, std::vector<std::string>> m_docTokens;
};
