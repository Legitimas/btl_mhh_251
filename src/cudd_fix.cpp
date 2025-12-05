// File: cudd_fix.cpp
#include <cstdio>
#include <ctime>

// Giả lập các hàm Linux mà CUDD cần nhưng Windows không có
extern "C" {

// 1. Hàm tính thời gian CPU (thay thế cho sys/times.h)
long util_cpu_time(void) { return (long)clock(); }

// 2. Hàm lấy giới hạn bộ nhớ (thay thế cho sys/resource.h)
// Trả về số lớn tượng trưng cho "vô cực"
unsigned long getSoftDataLimit(void) { return 2147483647; }
}