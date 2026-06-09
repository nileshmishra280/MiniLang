#include "symbol_table.h"
#include <stdexcept>

void SymbolTable::pushScope() {
    scopes.push_back({});
}

void SymbolTable::popScope() {
    if (scopes.empty()) throw std::runtime_error("popScope on empty scope stack");
    scopes.pop_back();
}

void SymbolTable::define(const std::string& name, Symbol sym) {
    if (scopes.empty()) throw std::runtime_error("No active scope");
    auto& top = scopes.back();
    if (top.count(name))
        throw std::runtime_error("Redefinition of '" + name + "'");
    top[name] = std::move(sym);
}

const Symbol& SymbolTable::lookup(const std::string& name) const {
    // Walk scopes from innermost outward
    for (int i = (int)scopes.size() - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    throw std::runtime_error("Undefined variable '" + name + "'");
}

bool SymbolTable::exists(const std::string& name) const {
    for (int i = (int)scopes.size() - 1; i >= 0; --i)
        if (scopes[i].count(name)) return true;
    return false;
}

int SymbolTable::allocLocal() {
    return nextSlot++;
}

void SymbolTable::resetLocals() {
    nextSlot = 0;
}
