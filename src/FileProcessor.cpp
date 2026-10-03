/**
 * @file FileProcessor.cpp
 * @brief Implements file reading with RAII stream management.
 *
 * RAII (Resource Acquisition Is Initialization):
 *   - std::ifstream acquires a file descriptor in its constructor.
 *   - When the ifstream goes out of scope (at the closing '}' of readFile),
 *     its destructor calls close() on the file descriptor automatically.
 *   - If an exception is thrown between open and end-of-function, the stack
 *     unwinds and the destructor still runs → the file is never leaked.
 *   - This is the C++ alternative to finally{} blocks in Java/Python.
 */

#include "FileProcessor.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

std::string FileProcessor::readFile(const std::filesystem::path& filePath) const
{
    // Validate extension before attempting to open.
    if (!isValidTxtFile(filePath)) {
        throw std::runtime_error(
            "FileProcessor: not a valid .txt file: " + filePath.string());
    }

    // RAII: ifstream constructor opens the file; destructor closes it.
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error(
            "FileProcessor: cannot open file: " + filePath.string());
    }

    // Read entire file content into a string via stringstream.
    // This is idiomatic C++ for small-to-medium files.
    std::ostringstream buffer;
    buffer << file.rdbuf();      // rdbuf() returns the underlying stream buffer

    std::string content = buffer.str();

    if (content.empty()) {
        throw std::runtime_error(
            "FileProcessor: file is empty: " + filePath.string());
    }

    // file destructor runs here → OS file handle released automatically
    return content;
}

std::vector<std::filesystem::path>
FileProcessor::listTxtFiles(const std::filesystem::path& dirPath) const
{
    if (!std::filesystem::is_directory(dirPath)) {
        throw std::runtime_error(
            "FileProcessor: not a directory: " + dirPath.string());
    }

    std::vector<std::filesystem::path> result;

    // Range-based for loop over directory entries (C++17 std::filesystem).
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            result.push_back(entry.path());
        }
    }

    // Sort alphabetically so load order is deterministic.
    std::sort(result.begin(), result.end());
    return result;
}

bool FileProcessor::isValidTxtFile(const std::filesystem::path& filePath) const
{
    return std::filesystem::exists(filePath) &&
           std::filesystem::is_regular_file(filePath) &&
           filePath.extension() == ".txt";
}
