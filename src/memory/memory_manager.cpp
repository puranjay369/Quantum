#include "memory_manager.h"

void MemoryTracker::trackHeapVar(const std::string& name) {
    heapVars.insert(name);
}

void MemoryTracker::markFreed(const std::string& name) {
    freedVars.insert(name);
}

bool MemoryTracker::isHeap(const std::string& name) {
    return heapVars.count(name) > 0;
}

void MemoryTracker::checkNotFreed(const std::string& name, int line) {
    if (freedVars.count(name) > 0) {
        throw std::runtime_error(
            "Memory Error at line " + std::to_string(line) +
            ": use of freed variable '" + name + "'."
        );
    }
}