cmake_minimum_required(VERSION 3.16)
project(minicc VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(include)

# Compiler sources (lexer → parser → codegen)
set(COMPILER_SOURCES
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/codegen.cpp
    src/optimizer.cpp
    src/symbol_table.cpp
)

# VM sources
set(VM_SOURCES
    src/vm.cpp
)

# minicc — the compiler binary
add_executable(minicc
    src/main.cpp
    ${COMPILER_SOURCES}
    ${VM_SOURCES}
)

# minivm — the virtual machine binary
add_executable(minivm
    src/vm_main.cpp
    ${VM_SOURCES}
)

# Tests
enable_testing()

add_executable(test_lexer    tests/test_lexer.cpp    ${COMPILER_SOURCES})
add_executable(test_parser   tests/test_parser.cpp   ${COMPILER_SOURCES})
add_executable(test_codegen  tests/test_codegen.cpp  ${COMPILER_SOURCES})
add_executable(test_vm       tests/test_vm.cpp       ${VM_SOURCES})
add_executable(test_e2e      tests/test_e2e.cpp      ${COMPILER_SOURCES} ${VM_SOURCES})

add_test(NAME lexer   COMMAND test_lexer)
add_test(NAME parser  COMMAND test_parser)
add_test(NAME codegen COMMAND test_codegen)
add_test(NAME vm      COMMAND test_vm)
add_test(NAME e2e     COMMAND test_e2e)
