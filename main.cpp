#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "lexer/lexer.hpp"

bool hasPDExtension(const std::string& filename) {
    if (filename.size() < 3) {
        return false;
    }
    return (filename.substr(filename.size() - 3) == ".pd");
}

std::string getVersion() {
    return "ppc : v1.0.0\n"
           "https://github.com/PranishDhital/PPC\n";
}

bool tokensToWindowsAssembly(const std::vector<Token>& tokens, std::string& assembly) {
    // This first version supports one statement: return <integer>;
    if (tokens.size() != 4 ||
        tokens[0].type != TokenType::PD_RETURN ||
        tokens[1].type != TokenType::PD_INT_LITERAL ||
        tokens[2].type != TokenType::PD_SEMI ||
        tokens[3].type != TokenType::PD_END) {
        std::cerr << "Error: expected one statement in the form 'return <integer>;'.\n";
        return false;
    }

    assembly = "global main\n"
               "section .text\n"
               "main:\n"
               "    mov eax, " + tokens[1].value + "\n"
               "    ret\n";
    return true;
}
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>.pd" << "\n";
        return 1;
    }

    std::string filename = argv[1];
    if (filename == "--version" || filename == "--V" || filename == "--v") {
        std::cout << getVersion() << "\n ";
        return 0;
    }

    if (!hasPDExtension(filename)) {
        std::cerr << "Error! .pd files are only supported" << "\n";
        return 1;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error! couldn't open file " << filename << "\n";
        return 1;
    }
    lexer lex;
    std::vector<Token> tokens;
    int row = 1;
    int column = 1;
    Token tok;
    do {
        tok = lex.getNextToken(file, row, column);
        lex.print(tok);
        tokens.push_back(tok);
    } while (tok.type != TokenType::PD_END);
    std::string assembly;
    if (!tokensToWindowsAssembly(tokens, assembly)) {
        return 1;
    }

    {
        std::ofstream assemblyFile("out.asm");
        if (!assemblyFile) {
            std::cerr << "Error: couldn't write out.asm\n";
            return 1;
        }
        assemblyFile << assembly;
        if (!assemblyFile) {
            std::cerr << "Error: failed while writing out.asm\n";
            return 1;
        }
    }
    std::cout << "Generated out.asm\n";

    if (std::system("nasm -f win64 out.asm -o out.obj") != 0) {
        std::cerr << "Error: NASM failed to assemble out.asm\n";
        return 1;
    }

    if (std::system("gcc out.obj -o out.exe") != 0) {
        std::cerr << "Error: GCC failed to link out.obj\n";
        return 1;
    }

    std::cout << "Built out.exe\n";
    return 0;
}
