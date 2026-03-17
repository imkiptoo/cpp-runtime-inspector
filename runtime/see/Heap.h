//! @file see/Heap.h
//! @brief Heap allocation tracking for the See++ runtime.

#pragma once

#include "TypeInfo.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace see {

//! Represents a single heap allocation.
struct Allocation {
    void* base;                       //!< Base address of allocation
    size_t size;                      //!< Size in bytes
    const TypeDescriptor* type;       //!< Type descriptor (may be nullptr for malloc)
    bool is_array;                    //!< True if allocated with new[]
    size_t array_count;               //!< Number of elements (for arrays)
    int heap_id;                      //!< Monotonic ID, never reused
    bool freed;                       //!< True if freed but still tracked
    int freed_at_step;                //!< Step number when freed (-1 if live)
    int allocated_at_step;            //!< Step number when allocated
};

//! Result of resolving a pointer against the heap.
struct ResolveResult {
    const Allocation* allocation;     //!< The allocation containing this address
    size_t offset;                    //!< Byte offset from allocation base
};

//! Tracks live heap allocations using a sorted vector.
//!
//! For educational programs with typically <1000 allocations, a sorted vector
//! with binary search is sufficient. Can be upgraded to a proper interval tree
//! if performance testing shows issues.
class HeapTracker {
public:
    //! Get the singleton instance.
    static HeapTracker& instance();

    //! Record a new allocation.
    //! @param base Base address of the allocation.
    //! @param size Size in bytes.
    //! @param type Type descriptor (may be nullptr for untyped allocations).
    //! @param is_array True if allocated with new[].
    //! @param array_count Number of array elements (1 for non-arrays).
    //! @param step Current step number.
    //! @return The heap ID assigned to this allocation.
    int insert(void* base, size_t size, const TypeDescriptor* type,
               bool is_array, size_t array_count, int step);

    //! Resolve a pointer to its containing allocation.
    //! @param ptr Pointer to resolve.
    //! @return ResolveResult if ptr falls within a tracked allocation, nullopt otherwise.
    std::optional<ResolveResult> resolve(const void* ptr) const;

    //! Mark an allocation as freed.
    //! @param base Base address of the allocation to free.
    //! @param step Current step number.
    //! @return True if the allocation was found and marked, false otherwise.
    bool markFreed(void* base, int step);

    //! Remove old freed allocations to bound memory usage.
    //! @param current_step Current step number.
    //! @param grace_period Steps to keep freed allocations for use-after-free detection.
    void sweep(int current_step, int grace_period = 100);

    //! Get all current allocations (including freed but not yet swept).
    const std::vector<Allocation>& getAllocations() const { return m_allocations; }

    //! Get all live (non-freed) allocations.
    std::vector<const Allocation*> getLiveAllocations() const;

    //! Get all leaked allocations (live at program end).
    std::vector<const Allocation*> getLeakedAllocations() const;

    //! Update type info for a malloc'd allocation when assigned to typed pointer.
    //! @param base Base address of the allocation.
    //! @param type Type descriptor to set.
    //! @param array_count Number of elements if array.
    void backfillType(void* base, const TypeDescriptor* type, size_t array_count = 1);

    //! Check if a pointer points to freed memory.
    //! @param ptr Pointer to check.
    //! @return True if ptr is in a freed allocation.
    bool isFreed(const void* ptr) const;

    //! Reset all state (for testing).
    void reset();

private:
    HeapTracker() = default;

    //! Find allocation containing address using binary search.
    //! @return Index into m_allocations, or -1 if not found.
    int findContaining(const void* ptr) const;

    //! Keep allocations sorted by base address.
    void sortAllocations();

    std::vector<Allocation> m_allocations;
    int m_nextHeapId = 1;
};

} // namespace see
