#include "vm.h"
#include <iostream>

int main() {
    std::cout << "minivm: use 'minicc --run <file.mc>' for the full pipeline.\n";
    std::cout << "        use 'minicc --dump-asm <file.mc>' to inspect bytecode.\n";
    return 0;
}
