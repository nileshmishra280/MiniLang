#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "optimizer.h"
#include "vm.h"
#include "ast.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

void printAST(ASTNode& root, std::ostream& out);  // defined in ast.cpp

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options] <source.mc>\n"
              << "\nOptions:\n"
              << "  --run          Compile and run immediately (default)\n"
              << "  --dump-tokens  Print token stream and exit\n"
              << "  --dump-ast     Print AST and exit\n"
              << "  --dump-asm     Print bytecode disassembly and exit\n"
              << "  --no-opt       Disable optimizer\n"
              << "  --trace        Trace VM execution\n"
              << "  -h, --help     Show this help\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    std::string sourceFile;
    bool dumpTokens = false;
    bool dumpAST    = false;
    bool dumpASM    = false;
    bool optimize   = true;
    bool traceVM    = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--dump-tokens") dumpTokens = true;
        else if (a == "--dump-ast")   dumpAST    = true;
        else if (a == "--dump-asm")   dumpASM    = true;
        else if (a == "--no-opt")     optimize   = false;
        else if (a == "--trace")      traceVM    = true;
        else if (a == "--run")        {}
        else if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        else if (a[0] != '-')         sourceFile = a;
        else { std::cerr << "Unknown option: " << a << "\n"; return 1; }
    }

    if (sourceFile.empty()) { usage(argv[0]); return 1; }

    // Read source
    std::ifstream f(sourceFile);
    if (!f) {
        std::cerr << "Cannot open '" << sourceFile << "'\n";
        return 1;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();

    try {
        // ── Lex ──────────────────────────────────────────────────────────
        Lexer lexer(src);
        auto tokens = lexer.tokenize();

        if (dumpTokens) {
            for (auto& t : tokens) std::cout << t.toString() << "\n";
            return 0;
        }

        // ── Parse ─────────────────────────────────────────────────────────
        Parser parser(std::move(tokens));
        auto program = parser.parse();

        if (dumpAST) {
            printAST(*program, std::cout);
            return 0;
        }

        // ── CodeGen ───────────────────────────────────────────────────────
        CodeGen cg;
        auto image = cg.generate(*program);

        // ── Optimize ──────────────────────────────────────────────────────
        if (optimize) {
            Optimizer opt;
            opt.optimize(image);
        }

        if (dumpASM) {
            VM::disassemble(image);
            return 0;
        }

        // ── Run ───────────────────────────────────────────────────────────
        VM vm(traceVM);
        return vm.run(image);

    } catch (LexError& e) {
        std::cerr << sourceFile << ":" << e.line << ":" << e.col
                  << ": Lex error: " << e.what() << "\n";
        return 1;
    } catch (ParseError& e) {
        std::cerr << sourceFile << ":" << e.line << ":" << e.col
                  << ": Parse error: " << e.what() << "\n";
        return 1;
    } catch (VMError& e) {
        std::cerr << "Runtime error at PC=" << e.pc << ": " << e.what() << "\n";
        return 1;
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
