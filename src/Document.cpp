/**
 * @file Document.cpp
 * @brief Implementation of the Document class.
 */

#include "Document.hpp"
#include <sstream>
#include <utility>

// ── Constructor ────────────────────────────────────────────────────────────

Document::Document(int id,
                   std::string title,
                   std::string path,
                   std::string content)
    : m_id(id)
    , m_title(std::move(title))    // std::move avoids a deep copy of the string
    , m_path(std::move(path))
    , m_content(std::move(content))
{
    // std::move transfers ownership of the string buffer rather than copying
    // character-by-character. After the move, the source string is in a valid
    // but unspecified state (we don't use it again, so this is fine).
}

// ── Accessors ──────────────────────────────────────────────────────────────

int Document::getID() const {
    return m_id;
}

const std::string& Document::getTitle() const {
    return m_title;   // Return by const-reference: no copy, caller cannot modify
}

const std::string& Document::getPath() const {
    return m_path;
}

const std::string& Document::getContent() const {
    return m_content;
}

// ── Utility ────────────────────────────────────────────────────────────────

std::size_t Document::wordCount() const {
    // Count whitespace-delimited tokens using a stringstream.
    // This is a rough metric – not used for ranking but useful for display.
    std::istringstream iss(m_content);
    std::size_t count = 0;
    std::string word;
    while (iss >> word) {
        ++count;
    }
    return count;
}
