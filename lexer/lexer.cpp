#include "lexer.hpp"

#include <cctype>
#include <iostream>

void lexer::print(const Token& token) {
    const char* name = "UNDEFINED";
    switch (token.type) {
        case TokenType::PD_RETURN:
            name = "RETURN";
            break;
        case TokenType::PD_SEMI:
            name = "SEMI";
            break;
        case TokenType::PD_IDENTIFIER:
            name = "IDENTIFIER";
            break;
        case TokenType::PD_INT_LITERAL:
            name = "INT_LITERAL";
            break;
        case TokenType::PD_FLOAT_LITERAL:
            name = "FLOAT_LITERAL";
            break;
        case TokenType::PD_UNDEFINED:
            break;
        case TokenType::PD_END:
            return;
    }
    std::cout << token.row << ':' << token.column << "  " << name << "      '" << token.value << "'"
              << "\n";
}

Token lexer::getNextToken(std::istream& file, int& row, int& column) {
    // Consume a character and update the location of the next character.
    auto read = [&]() {
        const char character = static_cast<char>(file.get());
        if (character == '\n') {
            ++row;
            column = 1;
        } else {
            ++column;
        }
        return character;
    };

    while (file && std::isspace(static_cast<unsigned char>(file.peek()))) {
        read();
    }

    if (!file || file.peek() == std::char_traits<char>::eof()) {
        return {TokenType::PD_END, "", row, column};
    }

    const int tokenRow = row;
    const int tokenColumn = column;
    const char character = read();

    if (character == ';') {
        return {TokenType::PD_SEMI, ";", tokenRow, tokenColumn};
    }

    if (std::isalpha(static_cast<unsigned char>(character)) || character == '_') {
        std::string identifier(1, character);
        while (file &&
               (std::isalnum(static_cast<unsigned char>(file.peek())) || file.peek() == '_')) {
            identifier += read();
        }
        const TokenType type =
            identifier == "return" ? TokenType::PD_RETURN : TokenType::PD_IDENTIFIER;
        return {type, identifier, tokenRow, tokenColumn};
    }

    if (std::isdigit(static_cast<unsigned char>(character))) {
        std::string number(1, character);
        while (file && std::isdigit(static_cast<unsigned char>(file.peek()))) {
            number += read();
        }

        bool isFloat = false;
        if (file && file.peek() == '.') {
            isFloat = true;
            number += read();
            while (file && std::isdigit(static_cast<unsigned char>(file.peek()))) {
                number += read();
            }
        }

        const TokenType type = isFloat ? TokenType::PD_FLOAT_LITERAL : TokenType::PD_INT_LITERAL;
        return {type, number, tokenRow, tokenColumn};
    }

    return {TokenType::PD_UNDEFINED, std::string(1, character), tokenRow, tokenColumn};
}
