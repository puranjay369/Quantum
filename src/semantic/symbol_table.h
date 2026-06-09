#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

// Represents the metadata of an identifier (variable or function).
struct SymbolInfo {
    std::string type;   // The resolved data type (e.g., "int", "float", "string").
    bool isFunction;    // True if callable function, false if variable.
    std::vector<std::string> paramTypes; // Expected parameter types for functions
};

// Manages nested lexical scopes and tracks declarations during semantic analysis.
class SymbolTable {
private:
    // Stack of scopes. back() is the innermost script, front() is global.
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;

public:
    SymbolTable();

    void enterScope();
    void exitScope();

    // Registers a new identifier in the CURRENT innermost scope.
    void define(const std::string& name, const std::string& type, bool isFunction = false, const std::vector<std::string>& paramTypes = {});

    // Looks up a symbol by name, searching from the innermost scope outwards.
    SymbolInfo lookup(const std::string& name) const;
    
    // Checks if a symbol exists ONLY in the current innermost scope.
    bool isDefinedInCurrentScope(const std::string& name) const;
};