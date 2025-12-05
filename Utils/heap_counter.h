#pragma once
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

// -------------------------------------------------
// Global atomic counter for heap bytes
// -------------------------------------------------
inline std::atomic<std::size_t> __total_heap_bytes{0};

// ----------------------------
// Global operator new/delete
// ----------------------------
inline void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    __total_heap_bytes += size;
    return ptr;
}

inline void operator delete(void* ptr, std::size_t size) noexcept {
    if (ptr) {
        __total_heap_bytes -= size;
        std::free(ptr);
    }
}

inline void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    __total_heap_bytes += size;
    return ptr;
}

inline void operator delete[](void* ptr, std::size_t size) noexcept {
    if (ptr) {
        __total_heap_bytes -= size;
        std::free(ptr);
    }
}

// Fallback delete operators
inline void operator delete(void* ptr) noexcept { std::free(ptr); }
inline void operator delete[](void* ptr) noexcept { std::free(ptr); }

// ----------------------------
// HeapTracker class
// ----------------------------
namespace Heap {

class HeapTracker {
   public:
    void start(const char* file, int line) {
        file_start_ = file;
        line_start_ = line;
        start_bytes_ = __total_heap_bytes.load();
    }

    void stop(const char* file, int line) {
        std::size_t end_bytes = __total_heap_bytes.load();
        std::size_t delta = end_bytes - start_bytes_;
        printf("\n[HeapCounter] %s:L%d - %s:L%d: %zu KB allocated\n",
               file_start_, line_start_, file, line, delta / 1024);
    }

   private:
    const char* file_start_ = nullptr;
    int line_start_ = 0;
    std::size_t start_bytes_ = 0;
};

// Global tracker instance inside namespace
inline HeapTracker __global_heap_tracker;

}  // namespace Heap

// Macros
#define HEAP_START() Heap::__global_heap_tracker.start(__FILE__, __LINE__)
#define HEAP_END() Heap::__global_heap_tracker.stop(__FILE__, __LINE__)