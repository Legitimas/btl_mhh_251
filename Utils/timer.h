// timer.h
#pragma once

#include <chrono>
#include <cstdio>
#include <string>

class Timer {
   public:
    void start(const char* file_start, int line_start) {
        file_start_ = file_start;
        line_start_ = line_start;
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    void stop(const char* file_end, int line_end) {
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start_time_)
                        .count();

        // print: "Timer (main.cpp:8 → main.cpp:12): 0.123 ms"
        printf("\n[Timer] %s:L%d - %s:L%d: %.4f ms\n", file_start_, line_start_,
               file_end, line_end, ms);
    }

   private:
    const char* file_start_ = nullptr;
    int line_start_ = 0;
    std::chrono::high_resolution_clock::time_point start_time_;
};

// global timer instance
inline Timer __global_timer;

// macros using __FILE__ and __LINE__
#define TIME_START() __global_timer.start(__FILE__, __LINE__)
#define TIME_END() __global_timer.stop(__FILE__, __LINE__)