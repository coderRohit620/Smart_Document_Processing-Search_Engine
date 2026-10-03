/**
 * @file TextProcessor.cpp
 * @brief Implements text normalization and tokenization.
 *
 * Normalization pipeline applied to every word:
 *   Step 1: Replace "C++" with "cpp" (special-case before lowering)
 *           Rationale: '+' is a valid programming character but invalid as
 *           an index token. Mapping → "cpp" preserves searchability.
 *   Step 2: Lowercase all characters.
 *   Step 3: Replace non-alphanumeric characters with a space character.
 *   Step 4: Split on whitespace.
 *   Step 5: Drop empty tokens.
 *   Step 6: Optionally drop stop-words.
 */

#include "TextProcessor.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <unordered_set>

// ── Stop-word list ─────────────────────────────────────────────────────────

/**
 * @brief Returns a reference to the static stop-word set.
 *
 * Using a function-local static means the set is constructed exactly once
 * (thread-safe since C++11) and lives for the duration of the program.
 * This avoids a global variable while keeping construction cheap.
 */
static const std::unordered_set<std::string>& getStopWords() {
    static const std::unordered_set<std::string> stopWords = {
        "the", "a", "an", "and", "or", "but", "in", "on", "at", "to",
        "for", "of", "with", "by", "from", "up", "about", "into", "is",
        "it", "its", "as", "be", "was", "are", "were", "been", "have",
        "has", "had", "do", "does", "did", "will", "would", "could",
        "should", "may", "might", "can", "this", "that", "these", "those",
        "he", "she", "they", "we", "you", "i", "me", "him", "her", "us",
        "not", "no", "nor", "so", "yet", "both", "either", "each",
        "than", "such", "when", "while", "if", "then", "because", "also",
        "very", "more", "most", "other", "some", "any", "all", "which"
    };
    return stopWords;
}

// ── Public API ─────────────────────────────────────────────────────────────

std::vector<std::string> TextProcessor::tokenize(const std::string& text,
                                                  bool removeStops)
{
    // Step 1: Replace "C++" → "cpp" before lowering (case-insensitive).
    // We do a manual pass because std::regex would pull in a large header.
    std::string processed = text;

    // Simple substring replacement for common C++ variants
    auto replaceCpp = [](std::string& s, const std::string& from,
                         const std::string& to) {
        std::size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replaceCpp(processed, "C++", "cpp");
    replaceCpp(processed, "c++", "cpp");

    // Step 2: Lowercase everything.
    std::transform(processed.begin(), processed.end(),
                   processed.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Step 3: Replace non-alphanumeric characters with spaces.
    // 'unsigned char' cast is required; tolower/isalnum expect non-negative values.
    for (char& c : processed) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            c = ' ';
        }
    }

    // Step 4 & 5: Tokenize on whitespace, skip empty tokens.
    std::vector<std::string> tokens;
    std::istringstream iss(processed);
    std::string token;
    while (iss >> token) {
        if (token.empty()) continue;

        // Step 6: Optionally filter stop-words.
        if (removeStops && isStopWord(token)) continue;

        tokens.push_back(std::move(token));
    }

    return tokens;
}

std::string TextProcessor::normalize(const std::string& word) {
    // Tokenize the single word with stop-words disabled (we want to normalise
    // the word itself, not accidentally discard it).
    auto tokens = tokenize(word, /*removeStops=*/false);
    if (tokens.empty()) return "";
    return tokens.front();
}

bool TextProcessor::isStopWord(const std::string& word) {
    return getStopWords().count(word) > 0;
}
