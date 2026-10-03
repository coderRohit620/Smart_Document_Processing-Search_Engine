/**
 * @file Index.hpp
 * @brief Abstract interface for any document index implementation.
 *
 * OOP Concepts Demonstrated:
 *
 *   ABSTRACTION
 *     - This class exposes WHAT operations an index supports without
 *       committing to HOW they are implemented.
 *     - Callers that depend only on Index* can be swapped to any concrete
 *       index (InvertedIndex, TrieIndex, etc.) without recompilation.
 *
 *   POLYMORPHISM
 *     - Pure virtual functions (`= 0`) force every derived class to provide
 *       its own implementation.
 *     - The virtual destructor is mandatory: destroying a derived object
 *       through a base pointer calls the correct destructor chain.
 *
 *   INHERITANCE
 *     - InvertedIndex inherits from Index and overrides each pure virtual.
 */

#pragma once

#include <string>
#include <vector>

// Forward declaration – avoids pulling in Document.hpp in every translation
// unit that only needs the Index interface.
class Document;

class Index {
public:
    // Virtual destructor: MUST be present so that `delete basePtr` correctly
    // calls the derived destructor when working with polymorphic objects.
    virtual ~Index() = default;

    /**
     * @brief Add a document to the index.
     * @param doc Reference to the document to index.
     */
    virtual void addDocument(const Document& doc) = 0;

    /**
     * @brief Remove a document from the index by its ID.
     * @param docID The unique identifier of the document to remove.
     */
    virtual void removeDocument(int docID) = 0;

    /**
     * @brief Retrieve the set of document IDs that contain the given term.
     * @param term A single normalized (lowercase, no punctuation) search term.
     * @return Vector of document IDs containing the term.
     */
    virtual std::vector<int> search(const std::string& term) const = 0;

    /**
     * @brief Rebuild the entire index from scratch.
     *        Useful after bulk additions/removals.
     */
    virtual void rebuild() = 0;

    /**
     * @brief Clear all entries from the index.
     */
    virtual void clear() = 0;

    /**
     * @brief Return the total number of unique terms in the index.
     */
    virtual std::size_t termCount() const = 0;
};
