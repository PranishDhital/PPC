#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "generation/generation.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

bool hasPDExtension(const std::string& filename) {
    return filename.size() >= 3 && filename.substr(filename.size() - 3) == ".pd";
}

std::string getVersion() {
    return "ppc : v1.0.0\n"
           "https://github.com/PranishDhital/PPC\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>.pd\n";
        return 1;
    }

    const std::string filename = argv[1];
    if (filename == "--version" || filename == "--V" || filename == "--v") {
        std::cout << getVersion();
        return 0;
    }

    if (!hasPDExtension(filename)) {
        std::cerr << "Error! .pd files are only supported\n";
        return 1;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error! couldn't open file " << filename << '\n';
        return 1;
    }

    lexer lex;
    std::vector<Token> tokens;
    int row = 1;
    int column = 1;
    Token token;
    do {
        token = lex.getNextToken(file, row, column);
        lex.print(token);
        tokens.push_back(token);
    } while (token.type != TokenType::PD_END);

    Parser parser(std::move(tokens));
    const auto tree = parser.parse();
    if (!tree) {
        std::cerr << "No valid exit statement found\n";
        return 1;
    }

    const Generator generator(*tree);
    {
        std::ofstream assemblyFile("out.asm");
        if (!assemblyFile) {
            std::cerr << "Error: couldn't write out.asm\n";
            return 1;
        }

        assemblyFile << generator.generate();
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

    if (std::system("gcc out.obj -o out.exe -mconsole") != 0) {
        std::cerr << "Error: GCC failed to link out.obj\n";
        return 1;
    }

    std::cout << "Built out.exe\n";
    return 0;
}
