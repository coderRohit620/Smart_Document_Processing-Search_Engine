/**
 * @file FileProcessor.hpp
 * @brief Reads text files from disk.
 *
 * RAII Demonstration:
 *   - std::ifstream is a RAII wrapper around a file descriptor.
 *   - When the ifstream object goes out of scope (end of readFile()),
 *     its destructor automatically closes the underlying OS file handle.
 *   - No explicit fclose() or cleanup code is needed, and the file is
 *     guaranteed to close even if an exception is thrown.
 *
 * This class is kept intentionally thin – it owns only the logic of
 * reading bytes from disk, leaving parsing/tokenization to TextProcessor.
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

class FileProcessor {
public:
    FileProcessor() = default;

    /**
     * @brief Read the entire contents of a .txt file.
     *
     * @param filePath Path to the file.
     * @return Full file content as a string.
     * @throws std::runtime_error if the file cannot be opened or is not .txt.
     */
    std::string readFile(const std::filesystem::path& filePath) const;

    /**
     * @brief Collect all .txt file paths within a directory (non-recursive).
     *
     * @param dirPath Path to the directory to scan.
     * @return Vector of paths to .txt files found.
     * @throws std::runtime_error if dirPath is not a valid directory.
     */
    std::vector<std::filesystem::path>
    listTxtFiles(const std::filesystem::path& dirPath) const;

    /**
     * @brief Return true if the path exists and has the .txt extension.
     */
    bool isValidTxtFile(const std::filesystem::path& filePath) const;
};
