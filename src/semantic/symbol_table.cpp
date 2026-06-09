#include "symbol_table.h"

SymbolTable::SymbolTable() {
    // Always start with one active scope (global scope)
    scopes.push_back(std::unordered_map<std::string, SymbolInfo>());
}

void SymbolTable::enterScope() {
    scopes.push_back(std::unordered_map<std::string, SymbolInfo>());
}

void SymbolTable::exitScope() {
    if (scopes.size() <= 1) {
        throw std::runtime_error("Compiler Error: Attempted to pop the global scope.");
    }
    scopes.pop_back();
}

void SymbolTable::define(const std::string& name, const std::string& type, bool isFunction, const std::vector<std::string>& paramTypes) {
    auto& currentScope = scopes.back();
    
    // Prevent redefining in the exact same scope block
    if (currentScope.find(name) != currentScope.end()) {
        throw std::runtime_error("Semantic Error: Symbol '" + name + "' is already defined in this scope.");
    }
    
    currentScope[name] = SymbolInfo{type, isFunction, paramTypes};
}

SymbolInfo SymbolTable::lookup(const std::string& name) const {
    // Search backward from innermost scope to global scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    
    throw std::runtime_error("Semantic Error: Undefined symbol '" + name + "'.");
}

bool SymbolTable::isDefinedInCurrentScope(const std::string& name) const {
    const auto& currentScope = scopes.back();
    return currentScope.find(name) != currentScope.end();
}