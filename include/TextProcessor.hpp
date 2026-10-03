/**
 * @file TextProcessor.hpp
 * @brief Stateless utility class for normalizing and tokenizing text.
 *
 * Responsibilities:
 *   - Convert raw text to lowercase.
 *   - Strip punctuation and special characters (keeping alphanumeric + spaces).
 *   - Handle C++ tokens like "C++" by converting '+' to 'p' → "cpp".
 *   - Split text into a list of non-empty word tokens.
 *   - Optionally filter common stop-words (the, is, a, …).
 *
 * Design Notes:
 *   - All methods are static; TextProcessor carries no mutable state.
 *   - A stop-word set is built once (lazy initialization) inside the .cpp
 *     and shared across all calls via a function-local static.
 *
 * Normalization decision (documented for interview):
 *   "C++" is a widely searched term. Rather than discarding '+', we map it to
 *   'p' so "C++" becomes "cpp". This is documented here so the choice can be
 *   defended during an interview.
 */

#pragma once

#include <string>
#include <vector>

class TextProcessor {
public:
    /**
     * @brief Tokenize raw text into normalized, lowercase words.
     *
     * Steps performed internally:
     *   1. "C++" → "cpp"  (special-case before lowering)
     *   2. lowercase all characters
     *   3. replace non-alphanumeric characters with space
     *   4. split on whitespace
     *   5. drop empty tokens
     *   6. optionally drop stop-words
     *
     * @param text         Raw input string.
     * @param removeStops  If true, filter common English stop-words.
     * @return             Vector of normalized tokens.
     */
    static std::vector<std::string> tokenize(const std::string& text,
                                             bool removeStops = true);

    /**
     * @brief Normalize a single word: lowercase + strip punctuation.
     *        Used to normalize individual query terms.
     */
    static std::string normalize(const std::string& word);

    /**
     * @brief Return true if the word is in the stop-word list.
     */
    static bool isStopWord(const std::string& word);

private:
    TextProcessor() = delete; // Prevent instantiation – pure utility class
};
