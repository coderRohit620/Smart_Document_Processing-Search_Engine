/**
 * @file Document.hpp
 * @brief Represents a single text document in the search engine.
 *
 * OOP Concept: ENCAPSULATION
 *   - All member variables are private.
 *   - Public interface provides controlled access via getters.
 *   - const-correct getters ensure callers cannot modify internal state.
 *
 * C++ Feature: Rule of 0
 *   - No custom destructor, copy/move constructors, or assignment operators
 *     are defined. The compiler-generated ones are correct because all members
 *     are standard types (int, std::string) that manage themselves.
 *   - This is the preferred approach for production classes.
 */

#pragma once

#include <string>

class Document {
public:
    /**
     * @brief Constructs a Document.
     * @param id      Unique numeric identifier assigned by DocumentManager.
     * @param title   Human-readable name (usually the file's base name).
     * @param path    Absolute or relative path to the source file.
     * @param content Full raw text content of the file.
     *
     * 'explicit' prevents accidental implicit conversions from int → Document.
     */
    explicit Document(int id,
                      std::string title,
                      std::string path,
                      std::string content);

    // ── Accessors ──────────────────────────────────────────────────────────
    int                getID()      const;
    const std::string& getTitle()   const;
    const std::string& getPath()    const;
    const std::string& getContent() const;

    /**
     * @brief Returns word count (whitespace-separated) as a quick size metric.
     */
    std::size_t wordCount() const;

private:
    int         m_id;       ///< Unique document identifier
    std::string m_title;    ///< Display name (e.g., "cpp.txt")
    std::string m_path;     ///< File-system path
    std::string m_content;  ///< Raw textual content
};
