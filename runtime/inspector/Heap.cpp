//! @file see/Heap.cpp
//! @brief Heap allocation tracking implementation.

#include "Heap.h"

#include <algorithm>

namespace inspector {

HeapTracker& HeapTracker::instance() {
    // Heap-allocated singleton to survive atexit handlers
    static HeapTracker* tracker = new HeapTracker();
    return *tracker;
}

int HeapTracker::insert(void* base, size_t size, const TypeDescriptor* type,
                        bool is_array, size_t array_count, int step) {
    Allocation alloc;
    alloc.base = base;
    alloc.size = size;
    alloc.type = type;
    alloc.is_array = is_array;
    alloc.array_count = array_count;
    alloc.heap_id = m_nextHeapId++;
    alloc.freed = false;
    alloc.freed_at_step = -1;
    alloc.allocated_at_step = step;

    m_allocations.push_back(alloc);
    sortAllocations();

    return alloc.heap_id;
}

std::optional<ResolveResult> HeapTracker::resolve(const void* ptr) const {
    int idx = findContaining(ptr);
    if (idx < 0) {
        return std::nullopt;
    }

    const Allocation& alloc = m_allocations[idx];
    size_t offset = static_cast<const char*>(ptr) - static_cast<const char*>(alloc.base);

    ResolveResult result;
    result.allocation = &alloc;
    result.offset = offset;
    return result;
}

bool HeapTracker::markFreed(void* base, int step) {
    for (auto& alloc : m_allocations) {
        if (alloc.base == base && !alloc.freed) {
            alloc.freed = true;
            alloc.freed_at_step = step;
            return true;
        }
    }
    return false;
}

void HeapTracker::sweep(int current_step, int grace_period) {
    m_allocations.erase(
        std::remove_if(m_allocations.begin(), m_allocations.end(),
                       [current_step, grace_period](const Allocation& alloc) {
                           return alloc.freed &&
                                  (current_step - alloc.freed_at_step) > grace_period;
                       }),
        m_allocations.end());
}

std::vector<const Allocation*> HeapTracker::getLiveAllocations() const {
    std::vector<const Allocation*> result;
    for (const auto& alloc : m_allocations) {
        if (!alloc.freed) {
            result.push_back(&alloc);
        }
    }
    return result;
}

std::vector<const Allocation*> HeapTracker::getLeakedAllocations() const {
    // Same as live allocations at program end
    return getLiveAllocations();
}

void HeapTracker::backfillType(void* base, const TypeDescriptor* type,
                                size_t array_count) {
    for (auto& alloc : m_allocations) {
        if (alloc.base == base && !alloc.type) {
            alloc.type = type;
            if (array_count > 1) {
                alloc.is_array = true;
                alloc.array_count = array_count;
            }
            return;
        }
    }
}

bool HeapTracker::isFreed(const void* ptr) const {
    int idx = findContaining(ptr);
    if (idx < 0) {
        return false;
    }
    return m_allocations[idx].freed;
}

void HeapTracker::reset() {
    m_allocations.clear();
    m_nextHeapId = 1;
}

int HeapTracker::findContaining(const void* ptr) const {
    if (m_allocations.empty()) {
        return -1;
    }

    // Binary search for the allocation whose base is <= ptr
    auto it = std::upper_bound(
        m_allocations.begin(), m_allocations.end(), ptr,
        [](const void* p, const Allocation& alloc) {
            return p < alloc.base;
        });

    // upper_bound returns iterator to first element > ptr
    // We need to check the element before it (if any)
    if (it == m_allocations.begin()) {
        return -1;
    }
    --it;

    // Check if ptr falls within this allocation
    const char* base = static_cast<const char*>(it->base);
    const char* end = base + it->size;
    const char* p = static_cast<const char*>(ptr);

    if (p >= base && p < end) {
        return static_cast<int>(it - m_allocations.begin());
    }

    return -1;
}

void HeapTracker::sortAllocations() {
    std::sort(m_allocations.begin(), m_allocations.end(),
              [](const Allocation& a, const Allocation& b) {
                  return a.base < b.base;
              });
}

} // namespace inspector
