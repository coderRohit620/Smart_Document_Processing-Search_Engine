/**
 * @file DocumentManager.cpp
 * @brief Manages the lifecycle of Document objects.
 *
 * Ownership model:
 *   - Every Document is heap-allocated and owned by a unique_ptr inside
 *     m_documents.
 *   - When removeDocument() calls m_documents.erase(), the unique_ptr in the
 *     map is destroyed, which in turn destructs and frees the Document.
 *   - No manual delete is ever written → zero risk of forgetting to free.
 */

#include "DocumentManager.hpp"
#include <iostream>
#include <stdexcept>
#include <algorithm>

// ── Constructor ────────────────────────────────────────────────────────────

DocumentManager::DocumentManager(std::filesystem::path dataDir)
    : m_dataDir(std::move(dataDir))
{}

// ── Public API ─────────────────────────────────────────────────────────────

int DocumentManager::loadFromDirectory() {
    std::vector<std::filesystem::path> files;

    try {
        files = m_fileProcessor.listTxtFiles(m_dataDir);
    } catch (const std::exception& ex) {
        std::cerr << "[DocumentManager] Cannot list directory '"
                  << m_dataDir << "': " << ex.what() << '\n';
        return 0;
    }

    int loaded = 0;
    for (const auto& path : files) {
        try {
            if (addDocument(path) != nullptr) {
                ++loaded;
            }
        } catch (const std::exception& ex) {
            std::cerr << "[DocumentManager] Skipping '" << path.filename()
                      << "': " << ex.what() << '\n';
        }
    }
    return loaded;
}

const Document* DocumentManager::addDocument(const std::filesystem::path& filePath)
{
    // Guard: duplicate check (same file title already loaded).
    std::string title = filePath.filename().string();
    if (documentExists(title)) {
        throw std::runtime_error(
            "Document '" + title + "' is already loaded.");
    }

    // FileProcessor reads the raw content; exceptions propagate to caller.
    std::string content = m_fileProcessor.readFile(filePath);

    return createDocument(filePath, content);
}

bool DocumentManager::removeDocument(int docID) {
    auto it = m_documents.find(docID);
    if (it == m_documents.end()) {
        return false;
    }
    m_documents.erase(it);
    // unique_ptr destructor deletes the Document automatically.
    return true;
}

const Document* DocumentManager::getDocument(int docID) const {
    auto it = m_documents.find(docID);
    if (it == m_documents.end()) {
        return nullptr;
    }
    return it->second.get(); // .get() returns the raw pointer without releasing ownership
}

std::vector<const Document*> DocumentManager::getAllDocuments() const {
    std::vector<const Document*> docs;
    docs.reserve(m_documents.size());

    for (const auto& [id, docPtr] : m_documents) {
        docs.push_back(docPtr.get());
    }

    // Sort by document ID for consistent display order.
    std::sort(docs.begin(), docs.end(),
              [](const Document* a, const Document* b) {
                  return a->getID() < b->getID();
              });

    return docs;
}

// ── Queries ────────────────────────────────────────────────────────────────

bool DocumentManager::documentExists(int docID) const {
    return m_documents.count(docID) > 0;
}

bool DocumentManager::documentExists(const std::string& title) const {
    for (const auto& [id, docPtr] : m_documents) {
        if (docPtr->getTitle() == title) return true;
    }
    return false;
}

std::size_t DocumentManager::documentCount() const {
    return m_documents.size();
}

void DocumentManager::printSummary() const {
    std::cout << "\n=== Document Collection (" << documentCount()
              << " documents) ===\n";

    auto docs = getAllDocuments();
    for (const Document* doc : docs) {
        std::cout << "  [" << doc->getID() << "] "
                  << doc->getTitle()
                  << "  (" << doc->wordCount() << " words)\n";
    }
    std::cout << '\n';
}

// ── Data Directory ─────────────────────────────────────────────────────────

void DocumentManager::setDataDirectory(const std::filesystem::path& dir) {
    m_dataDir = dir;
}

const std::filesystem::path& DocumentManager::getDataDirectory() const {
    return m_dataDir;
}

// ── Private Helpers ────────────────────────────────────────────────────────

const Document* DocumentManager::createDocument(
    const std::filesystem::path& filePath,
    const std::string& content)
{
    int id = m_nextID++;

    // std::make_unique<Document>(...) allocates a Document on the heap and
    // wraps it in a unique_ptr.  No raw 'new' required.
    auto docPtr = std::make_unique<Document>(
        id,
        filePath.filename().string(),   // title
        filePath.string(),              // full path
        content
    );

    // Store the unique_ptr; DocumentManager now owns the Document.
    const Document* rawPtr = docPtr.get();
    m_documents[id] = std::move(docPtr); // Transfer ownership into the map.

    return rawPtr; // Non-owning pointer – safe as long as DocumentManager lives.
}
