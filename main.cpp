#include <cstdlib>
#include <filesystem>
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
    const Token sourceStart{TokenType::PD_UNDEFINED, "", 1, 1};
    if (filename == "--version" || filename == "--V" || filename == "--v") {
        std::cout << getVersion();
        return 0;
    }

    if (!hasPDExtension(filename)) {
        reportError(filename, sourceStart, ".pd files are only supported");
        return 1;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        reportError(filename, sourceStart, "could not open source file");
        return 1;
    }

    lexer lex;
    std::vector<Token> tokens;
    int row = 1;
    int column = 1;
    Token token;
    do {
        token = lex.getNextToken(file, row, column);
        if (token.type == TokenType::PD_UNDEFINED) {
            reportError(filename, token, "unrecognized token '" + token.value + "'");
            return 1;
        }
        lex.print(token);
        tokens.push_back(token);
    } while (token.type != TokenType::PD_END);

    Parser parser(std::move(tokens), filename);
    std::optional<NodeProg> prog = parser.parse_prog();
    if (!prog) {
        return 1;
    }

    const Generator generator(*prog, filename);
    const std::filesystem::path outputDir = std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::create_directories(outputDir);
    const std::filesystem::path assemblyPath = outputDir / "out.asm";
    const std::filesystem::path objectPath = outputDir / "out.obj";
    const std::filesystem::path executablePath = outputDir / "out.exe";
    {
        std::ofstream assemblyFile(assemblyPath);
        if (!assemblyFile) {
            reportError(filename, sourceStart,
                        "could not write assembly output: " + assemblyPath.string());
            return 1;
        }

        assemblyFile << generator.gen_prog();
        if (!assemblyFile) {
            reportError(filename, sourceStart,
                        "failed while writing assembly output: " + assemblyPath.string());
            return 1;
        }
    }

    std::cout << "Generated " << assemblyPath.string() << '\n';

    const std::string assembleCommand =
        "nasm -f win64 \"" + assemblyPath.string() + "\" -o \"" + objectPath.string() + "\"";
    if (std::system(assembleCommand.c_str()) != 0) {
        reportError(filename, sourceStart, "NASM failed to assemble generated output");
        return 1;
    }

    const std::string linkCommand =
        "gcc \"" + objectPath.string() + "\" -o \"" + executablePath.string() + "\" -mconsole";
    if (std::system(linkCommand.c_str()) != 0) {
        reportError(filename, sourceStart, "GCC failed to link generated output");
        return 1;
    }

    std::cout << "Built " << executablePath.string() << '\n';
    return 0;
}
