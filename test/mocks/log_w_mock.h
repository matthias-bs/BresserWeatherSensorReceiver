#ifndef LOG_W_MOCK_H
#define LOG_W_MOCK_H


// Simple stub for log_w macro for unit testing: print to stdout
#include <cstdio>
#ifndef log_w
#define log_w(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#endif

#endif // LOG_W_MOCK_H
