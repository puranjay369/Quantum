#pragma once
#include <string>
#include <unordered_set>
#include <stdexcept>

class MemoryTracker {
public:
    void trackHeapVar(const std::string& name);
    void markFreed(const std::string& name);
    bool isHeap(const std::string& name);
    void checkNotFreed(const std::string& name, int line);

private:
    std::unordered_set<std::string> heapVars;
    std::unordered_set<std::string> freedVars;
};