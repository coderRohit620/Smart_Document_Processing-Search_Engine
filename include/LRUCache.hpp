/**
 * @file LRUCache.hpp
 * @brief Generic LRU (Least Recently Used) cache.
 *
 * ── Data Structure Design ──────────────────────────────────────────────────
 *
 * An LRU cache must satisfy two requirements:
 *   1. Fast lookup:  "Is key K cached?"  → O(1) average
 *   2. Order tracking: maintain usage order so we know which entry to evict.
 *
 * We combine TWO data structures to achieve this:
 *
 *   std::list<pair<Key, Value>>   (doubly-linked list)
 *     - Stores (key, value) pairs in recency order.
 *     - Most recently used (MRU) entry → front.
 *     - Least recently used (LRU) entry → back (eviction candidate).
 *     - Moving an existing node to the front is O(1) because std::list
 *       splice/iterator operations don't invalidate other iterators.
 *
 *   std::unordered_map<Key, ListIterator>   (hash map)
 *     - Maps each key to its iterator inside the list.
 *     - O(1) average lookup to find the node without scanning the list.
 *     - After lookup we jump directly to the node and splice it to front.
 *
 * ── Why O(1) Average? ──────────────────────────────────────────────────────
 *   - unordered_map get/set: O(1) average, O(n) worst case (hash collision).
 *   - list splice (move to front): O(1) always.
 *   - Combined: O(1) average per get/put, O(n) worst case due to hash table.
 *   (We say "average O(1)" to be precise about hash-table behaviour.)
 *
 * ── Template Design ────────────────────────────────────────────────────────
 *   Templated on <Key, Value> so the same implementation works for any
 *   cacheable type without code duplication.
 *   The entire implementation lives in this header because templates must be
 *   visible at instantiation time.
 */

#pragma once

#include <list>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <vector>

template <typename Key, typename Value>
class LRUCache {
public:
    /**
     * @brief Construct an LRU cache with a fixed capacity.
     * @param capacity Maximum number of entries before eviction occurs.
     * @throws std::invalid_argument if capacity == 0.
     */
    explicit LRUCache(std::size_t capacity)
        : m_capacity(capacity)
    {
        if (capacity == 0) {
            throw std::invalid_argument("LRUCache capacity must be > 0");
        }
    }

    // ── Core Operations ────────────────────────────────────────────────────

    /**
     * @brief Look up a key in the cache.
     *
     * On a cache HIT:
     *   - Move the entry to the front of the list (mark as most recently used).
     *   - Return a const reference via std::optional.
     *
     * On a cache MISS:
     *   - Return std::nullopt.
     *
     * Average time complexity: O(1)  (hash lookup + O(1) list splice)
     */
    std::optional<Value> get(const Key& key) {
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            return std::nullopt;   // Cache MISS
        }
        // Move accessed node to the front (most recently used position).
        m_list.splice(m_list.begin(), m_list, it->second);
        return it->second->second; // Cache HIT – return copy of value
    }

    /**
     * @brief Insert or update a key-value pair.
     *
     * If the key already exists: update value and move to front.
     * If the key is new and cache is full: evict the LRU entry (back of list),
     *   then insert the new entry at the front.
     *
     * Average time complexity: O(1)
     */
    void put(const Key& key, Value value) {
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            // Key exists – update value and move to front.
            it->second->second = std::move(value);
            m_list.splice(m_list.begin(), m_list, it->second);
            return;
        }

        // Evict LRU entry if at capacity.
        if (m_list.size() >= m_capacity) {
            // The back of the list is the least recently used entry.
            const Key& lruKey = m_list.back().first;
            m_map.erase(lruKey);   // Remove from hash map first
            m_list.pop_back();     // Remove from list
        }

        // Insert new entry at the front (most recently used).
        m_list.emplace_front(key, std::move(value));
        m_map[key] = m_list.begin();
    }

    /**
     * @brief Check whether a key is present without changing recency order.
     */
    bool contains(const Key& key) const {
        return m_map.count(key) > 0;
    }

    // ── Introspection ──────────────────────────────────────────────────────

    std::size_t size()     const { return m_list.size(); }
    std::size_t capacity() const { return m_capacity; }
    bool        empty()    const { return m_list.empty(); }

    /**
     * @brief Return keys in MRU-to-LRU order (for display/debugging).
     */
    std::vector<Key> keysInOrder() const {
        std::vector<Key> keys;
        keys.reserve(m_list.size());
        for (const auto& [k, v] : m_list) {
            keys.push_back(k);
        }
        return keys;
    }

    void clear() {
        m_list.clear();
        m_map.clear();
    }

private:
    // ── Type Aliases ───────────────────────────────────────────────────────
    using ListEntry   = std::pair<Key, Value>;
    using CacheList   = std::list<ListEntry>;
    using ListIter    = typename CacheList::iterator;
    using LookupTable = std::unordered_map<Key, ListIter>;

    // ── Members ────────────────────────────────────────────────────────────
    std::size_t m_capacity;  ///< Maximum number of cached entries
    CacheList   m_list;      ///< Doubly-linked list ordered MRU→LRU
    LookupTable m_map;       ///< Key → list iterator for O(1) lookup
};
