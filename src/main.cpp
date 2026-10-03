/**
 * @file main.cpp
 * @brief Command-line interface for the Smart Document Search Engine.
 *
 * Architecture wiring:
 *   DocumentManager  ──owns──►  Document objects (via unique_ptr)
 *        │
 *        ▼ (provides documents to)
 *   InvertedIndex    ──implements──►  Index (abstract interface)
 *        │
 *        ▼ (used by)
 *   SearchEngine     ──uses──►  Index& (polymorphism)
 *                    ──uses──►  LRUCache (recent queries)
 *
 * Object lifetimes:
 *   All major objects are stack-allocated (automatic storage duration).
 *   No manual new/delete in main.
 */

#include <iostream>
#include <string>
#include <limits>
#include <filesystem>

#include "DocumentManager.hpp"
#include "InvertedIndex.hpp"
#include "SearchEngine.hpp"
#include "Document.hpp"

// ── Helpers ────────────────────────────────────────────────────────────────

static void printMenu() {
    std::cout << "\n╔═══════════════════════════════════════════╗\n";
    std::cout <<   "║   Smart Document Processing & Search      ║\n";
    std::cout <<   "╠═══════════════════════════════════════════╣\n";
    std::cout <<   "║  1. Load documents from data/ directory   ║\n";
    std::cout <<   "║  2. List all documents                    ║\n";
    std::cout <<   "║  3. Add a document (file path)            ║\n";
    std::cout <<   "║  4. Remove a document (by ID)             ║\n";
    std::cout <<   "║  5. Search                                ║\n";
    std::cout <<   "║  6. Display index statistics              ║\n";
    std::cout <<   "║  7. Display cache information             ║\n";
    std::cout <<   "║  8. Exit                                  ║\n";
    std::cout <<   "╚═══════════════════════════════════════════╝\n";
    std::cout << "Choice: ";
}

static void printResults(const std::vector<SearchResult>& results,
                         const std::string& query,
                         bool cacheHit)
{
    std::cout << "\n── Search Results ─────────────────────────────────────\n";
    std::cout << "  Query : \"" << query << "\"\n";
    std::cout << "  Status: " << (cacheHit ? "[CACHE HIT]" : "[CACHE MISS]") << "\n\n";

    if (results.empty()) {
        std::cout << "  No matching documents found.\n";
        return;
    }

    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        std::cout << "  " << (i + 1) << ". " << r.title << "\n";
        std::cout << "     Document ID : " << r.docID   << "\n";
        std::cout << "     Score       : " << r.score   << "\n\n";
    }
}

// ── Main ───────────────────────────────────────────────────────────────────

int main() {
    // ── Create core components (stack-allocated) ──────────────────────────
    DocumentManager docManager("data");
    InvertedIndex   index;
    // SearchEngine interacts with the index via the abstract Index& interface.
    SearchEngine    engine(index, docManager, /*cacheCapacity=*/5);

    std::cout << "\n╔═══════════════════════════════════════════╗\n";
    std::cout <<   "║  Smart Document Processing & Search Engine ║\n";
    std::cout <<   "║  Portfolio Project  |  C++17               ║\n";
    std::cout <<   "╚════════════════════════════════════════════╝\n";
    std::cout << "\nTip: Select option 1 first to load sample documents.\n";

    int choice = 0;

    while (true) {
        printMenu();

        // Robust integer input – guards against non-numeric input.
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "\n[!] Invalid input. Please enter a number 1-8.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {

        // ── Option 1: Load documents ─────────────────────────────────────
        case 1: {
            // Re-build index from scratch when loading.
            index.clear();

            int loaded = docManager.loadFromDirectory();
            std::cout << "\n[✓] Loaded " << loaded << " document(s) from '"
                      << docManager.getDataDirectory().string() << "'\n";

            // Add each document to the inverted index.
            for (const Document* doc : docManager.getAllDocuments()) {
                index.addDocument(*doc);
            }
            std::cout << "[✓] Index built. " << index.termCount()
                      << " unique terms indexed.\n";
            break;
        }

        // ── Option 2: List documents ─────────────────────────────────────
        case 2: {
            if (docManager.documentCount() == 0) {
                std::cout << "\n[!] No documents loaded. Use option 1 first.\n";
            } else {
                docManager.printSummary();
            }
            break;
        }

        // ── Option 3: Add a document ─────────────────────────────────────
        case 3: {
            std::cout << "Enter file path (e.g., data/myfile.txt): ";
            std::string path;
            std::getline(std::cin, path);

            if (path.empty()) {
                std::cout << "\n[!] File path cannot be empty.\n";
                break;
            }

            try {
                const Document* doc = docManager.addDocument(path);
                if (doc) {
                    index.addDocument(*doc);
                    engine.clearCache(); // Invalidate cache; index changed.
                    std::cout << "[✓] Added '" << doc->getTitle()
                              << "' with ID " << doc->getID() << ".\n";
                }
            } catch (const std::exception& ex) {
                std::cout << "\n[!] Error: " << ex.what() << '\n';
            }
            break;
        }

        // ── Option 4: Remove a document ──────────────────────────────────
        case 4: {
            if (docManager.documentCount() == 0) {
                std::cout << "\n[!] No documents loaded.\n";
                break;
            }
            docManager.printSummary();
            std::cout << "Enter document ID to remove: ";
            int id;
            if (!(std::cin >> id)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "\n[!] Invalid ID.\n";
                break;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            index.removeDocument(id);
            if (docManager.removeDocument(id)) {
                engine.clearCache(); // Cache may reference removed document.
                std::cout << "[✓] Document " << id << " removed.\n";
            } else {
                std::cout << "\n[!] Document ID " << id << " not found.\n";
            }
            break;
        }

        // ── Option 5: Search ─────────────────────────────────────────────
        case 5: {
            if (docManager.documentCount() == 0) {
                std::cout << "\n[!] No documents loaded. Use option 1 first.\n";
                break;
            }
            std::cout << "Enter search query (e.g., \"memory cpp\"): ";
            std::string query;
            std::getline(std::cin, query);

            if (query.empty()) {
                std::cout << "\n[!] Query cannot be empty.\n";
                break;
            }

            try {
                bool cacheHit = false;
                auto results  = engine.search(query, cacheHit);
                printResults(results, query, cacheHit);
            } catch (const std::exception& ex) {
                std::cout << "\n[!] Search error: " << ex.what() << '\n';
            }
            break;
        }

        // ── Option 6: Index statistics ───────────────────────────────────
        case 6: {
            if (index.termCount() == 0) {
                std::cout << "\n[!] Index is empty.\n";
            } else {
                index.printStats();
            }
            break;
        }

        // ── Option 7: Cache information ──────────────────────────────────
        case 7: {
            engine.printCacheInfo();
            break;
        }

        // ── Option 8: Exit ───────────────────────────────────────────────
        case 8: {
            std::cout << "\nGoodbye!\n";
            return 0;
        }

        default: {
            std::cout << "\n[!] Invalid option. Please choose 1-8.\n";
            break;
        }

        } // end switch
    }   // end while

    return 0;
}
