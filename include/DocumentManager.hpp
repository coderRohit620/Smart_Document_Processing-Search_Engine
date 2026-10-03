/**
 * @file DocumentManager.hpp
 * @brief Manages the collection of documents in the system.
 *
 * OOP Concept: ENCAPSULATION
 *   - The document collection and ID counter are private.
 *   - The public interface provides controlled CRUD operations.
 *
 * Smart Pointer Usage:
 *   - Documents are stored as std::unique_ptr<Document>.
 *   - unique_ptr communicates SOLE OWNERSHIP: DocumentManager is the only
 *     owner of each Document object.
 *   - When a document is removed or DocumentManager is destroyed, the
 *     unique_ptr destructor automatically frees the heap memory.
 *   - No manual delete is needed, preventing memory leaks.
 *
 * Why not shared_ptr?
 *   - shared_ptr is for SHARED ownership (multiple owners of one object).
 *   - Here only DocumentManager owns each Document; shared_ptr would add
 *     reference-counting overhead with no benefit.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

#include "Document.hpp"
#include "FileProcessor.hpp"

class DocumentManager {
public:
    explicit DocumentManager(std::filesystem::path dataDir = "data");

    // ── Document CRUD ──────────────────────────────────────────────────────

    /**
     * @brief Load all .txt files from the configured data directory.
     * @return Number of documents successfully loaded.
     */
    int loadFromDirectory();

    /**
     * @brief Add a single document from a file path.
     * @param filePath Path to a .txt file.
     * @return Pointer to the newly created Document, or nullptr on failure.
     * @throws std::runtime_error if the file is invalid.
     */
    const Document* addDocument(const std::filesystem::path& filePath);

    /**
     * @brief Remove a document by its ID.
     * @return true if the document was found and removed.
     */
    bool removeDocument(int docID);

    /**
     * @brief Retrieve a document by its ID.
     * @return Pointer to the document, or nullptr if not found.
     */
    const Document* getDocument(int docID) const;

    /**
     * @brief Return all documents as a vector of raw pointers (non-owning).
     *        The caller must NOT delete these pointers.
     */
    std::vector<const Document*> getAllDocuments() const;

    // ── Queries ────────────────────────────────────────────────────────────
    bool        documentExists(int docID)             const;
    bool        documentExists(const std::string& title) const;
    std::size_t documentCount()                       const;
    void        printSummary()                        const;

    // ── Data Directory ─────────────────────────────────────────────────────
    void setDataDirectory(const std::filesystem::path& dir);
    const std::filesystem::path& getDataDirectory() const;

private:
    /**
     * @brief Create a Document object and assign it the next available ID.
     */
    const Document* createDocument(const std::filesystem::path& filePath,
                                   const std::string& content);

    // ── Members ────────────────────────────────────────────────────────────
    std::filesystem::path m_dataDir;

    // docID → owned Document
    // unique_ptr ensures automatic cleanup when entries are removed.
    std::unordered_map<int, std::unique_ptr<Document>> m_documents;

    int m_nextID{1};  ///< Monotonically increasing ID counter

    FileProcessor m_fileProcessor;  ///< Handles raw file I/O
};
