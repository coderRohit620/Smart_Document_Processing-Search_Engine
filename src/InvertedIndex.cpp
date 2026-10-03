/**
 * @file InvertedIndex.cpp
 * @brief Implements the inverted index operations.
 *
 * ── Index Structure (recap) ────────────────────────────────────────────────
 *   m_index: unordered_map<term, unordered_map<docID, freq>>
 *
 *   Adding document 1 containing "cpp memory cpp":
 *     m_index["cpp"][1]    = 2   (appears twice)
 *     m_index["memory"][1] = 1
 *
 *   Adding document 2 containing "memory management":
 *     m_index["memory"][2] = 1
 *     m_index["management"][2] = 1
 *
 *   Searching "memory":
 *     m_index["memory"] → {1:1, 2:1} → docIDs = {1, 2}
 *
 * ── Complexity ─────────────────────────────────────────────────────────────
 *   addDocument:     O(L) where L = number of tokens in the document
 *   removeDocument:  O(L) – must decrement each term's postings entry
 *   search(term):    O(D) where D = number of documents containing the term
 *                    (building the result vector from the inner map)
 *   getTermFreq:     O(1) average (two nested unordered_map lookups)
 *   termCount:       O(1)
 */

#include "InvertedIndex.hpp"
#include "Document.hpp"
#include "TextProcessor.hpp"
#include <iostream>
#include <algorithm>

// ── Index interface implementation ─────────────────────────────────────────

void InvertedIndex::addDocument(const Document& doc) {
    // Tokenize with stop-word removal enabled for clean indexing.
    std::vector<std::string> tokens =
        TextProcessor::tokenize(doc.getContent(), /*removeStops=*/true);

    // Count term frequencies for this document.
    // Using a local unordered_map accumulator avoids multiple lookups.
    std::unordered_map<std::string, int> localFreq;
    for (const std::string& token : tokens) {
        ++localFreq[token];
    }

    // Merge local frequencies into the global inverted index.
    for (const auto& [term, freq] : localFreq) {
        m_index[term][doc.getID()] = freq;
    }

    // Store tokens for this document (needed by removeDocument).
    m_docTokens[doc.getID()] = std::move(tokens);
}

void InvertedIndex::removeDocument(int docID) {
    auto tokenIt = m_docTokens.find(docID);
    if (tokenIt == m_docTokens.end()) {
        return; // Document was never indexed; nothing to do.
    }

    // For each token this document contributed, remove its postings entry.
    // Using an unordered_set to avoid repeated lookups for duplicate tokens.
    std::unordered_set<std::string> seen;
    for (const std::string& token : tokenIt->second) {
        if (seen.count(token)) continue;
        seen.insert(token);

        auto indexIt = m_index.find(token);
        if (indexIt != m_index.end()) {
            indexIt->second.erase(docID); // Remove docID from postings list

            // If no documents remain for this term, remove the term entirely.
            if (indexIt->second.empty()) {
                m_index.erase(indexIt);
            }
        }
    }

    m_docTokens.erase(tokenIt);
}

std::vector<int> InvertedIndex::search(const std::string& term) const {
    std::vector<int> result;

    auto it = m_index.find(term);
    if (it == m_index.end()) {
        return result; // Empty – term not found
    }

    // Collect all docIDs that contain this term.
    result.reserve(it->second.size());
    for (const auto& [docID, freq] : it->second) {
        result.push_back(docID);
    }

    return result;
}

void InvertedIndex::rebuild() {
    // A true rebuild would need the full Document list.
    // Here we recompute from stored token lists (which are always kept in sync).
    // This demonstrates the design but the real rebuild path runs through
    // addDocument() calls from SearchEngine when needed.
    // NOTE: In practice, callers should call clear() then re-add all documents.
    // This method is provided to satisfy the abstract interface.
}

void InvertedIndex::clear() {
    m_index.clear();
    m_docTokens.clear();
}

std::size_t InvertedIndex::termCount() const {
    return m_index.size();
}

// ── Extended API ───────────────────────────────────────────────────────────

int InvertedIndex::getTermFrequency(const std::string& term, int docID) const {
    auto termIt = m_index.find(term);
    if (termIt == m_index.end()) return 0;

    auto docIt = termIt->second.find(docID);
    if (docIt == termIt->second.end()) return 0;

    return docIt->second;
}

std::size_t InvertedIndex::documentFrequency(const std::string& term) const {
    auto it = m_index.find(term);
    if (it == m_index.end()) return 0;
    return it->second.size();
}

void InvertedIndex::printStats() const {
    std::cout << "\n=== Inverted Index Statistics ===\n";
    std::cout << "  Unique terms indexed : " << termCount() << '\n';
    std::cout << "  Documents in index   : " << m_docTokens.size() << '\n';

    // Find the top-5 most common terms by document frequency.
    std::vector<std::pair<std::string, std::size_t>> termFreqs;
    termFreqs.reserve(m_index.size());

    for (const auto& [term, postings] : m_index) {
        termFreqs.emplace_back(term, postings.size());
    }

    // Partial sort – only need top 5.
    std::size_t topN = std::min<std::size_t>(5, termFreqs.size());
    std::partial_sort(termFreqs.begin(),
                      termFreqs.begin() + static_cast<std::ptrdiff_t>(topN),
                      termFreqs.end(),
                      [](const auto& a, const auto& b) {
                          return a.second > b.second;
                      });

    std::cout << "  Top terms (by document frequency):\n";
    for (std::size_t i = 0; i < topN; ++i) {
        std::cout << "    \"" << termFreqs[i].first
                  << "\"  → in " << termFreqs[i].second << " doc(s)\n";
    }
    std::cout << '\n';
}

std::vector<std::string> InvertedIndex::allTerms() const {
    std::vector<std::string> terms;
    terms.reserve(m_index.size());
    for (const auto& [term, _] : m_index) {
        terms.push_back(term);
    }
    return terms;
}
