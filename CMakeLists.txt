cmake_minimum_required(VERSION 3.16)
project(SmartDocumentSearch VERSION 1.0.0 LANGUAGES CXX)

# ── C++17 standard ─────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # Disable compiler-specific extensions

# ── Compiler warnings ──────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Shared library of core modules ────────────────────────────────────────
# All engine source files are compiled into a static library so both the
# main executable and the test executable can link against the same objects
# without recompiling.
add_library(engine_lib STATIC
    src/Document.cpp
    src/DocumentManager.cpp
    src/TextProcessor.cpp
    src/FileProcessor.cpp
    src/InvertedIndex.cpp
    src/SearchEngine.cpp
)

target_include_directories(engine_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ── Main executable ────────────────────────────────────────────────────────
add_executable(SmartDocSearch src/main.cpp)
target_link_libraries(SmartDocSearch PRIVATE engine_lib)

# Copy data/ directory next to the executable so the app can find it at runtime.
add_custom_command(TARGET SmartDocSearch POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:SmartDocSearch>/data
    COMMENT "Copying data/ directory to build output"
)

# ── Test executable ────────────────────────────────────────────────────────
enable_testing()

add_executable(test_runner tests/test_runner.cpp)
target_link_libraries(test_runner PRIVATE engine_lib)
target_include_directories(test_runner PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Register with CTest so 'cmake --build . --target test' runs tests.
add_test(NAME AllTests COMMAND test_runner)

# ── Install (optional) ─────────────────────────────────────────────────────
install(TARGETS SmartDocSearch DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/SmartDocSearch/data)
