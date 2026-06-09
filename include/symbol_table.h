#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

struct Symbol {
    std::string type;
    int         slot;       // local slot index (or -1 for global)
    int         arraySize;  // 0 = scalar
    bool        isGlobal;
    bool        isParam;
};

class SymbolTable {
public:
    void pushScope();
    void popScope();

    // Define a symbol; throws if already defined in current scope
    void define(const std::string& name, Symbol sym);

    // Look up; throws if not found
    const Symbol& lookup(const std::string& name) const;

    bool exists(const std::string& name) const;

    int  allocLocal();   // returns next slot index
    void resetLocals();  // call at function entry
    int  localCount() const { return nextSlot; }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int nextSlot = 0;
};
