#pragma once
#include <cctype>
#include <iostream>
#include <istream>
#include <iterator>
#include <optional>
#include <string>

enum class TokenType {
    PD_RETURN,
    PD_EXIT,
    PD_SEMI,
    PD_IDENTIFIER,
    PD_INT_LITERAL,
    PD_FLOAT_LITERAL,
    PD_END,
    PD_UNDEFINED,
};

struct Token {
    TokenType type = TokenType::PD_UNDEFINED;
    std::string value;
    int row = 1;
    int column = 1;
};

class lexer {
   public:
    inline void print(const Token& token) {
        const char* name = "UNDEFINED";
        switch (token.type) {
            case TokenType::PD_RETURN:
                name = "RETURN";
                break;
            case TokenType::PD_EXIT:
                name = "EXIT";
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
        std::cout << token.row << ':' << token.column << "  " << name << "::  ' " << token.value
                  << " '\n";
    }

    inline Token getNextToken(std::istream& file, int& row, int& column) {
        if (!m_loaded) {
            m_src.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            m_loaded = true;
        }

        auto consumeAndTrackLocation = [&]() {
            const char character = consume();
            if (character == '\n') {
                ++row;
                column = 1;
            } else {
                ++column;
            }
            return character;
        };

        while (const auto character = peek()) {
            if (!std::isspace(static_cast<unsigned char>(*character))) {
                break;
            }
            consumeAndTrackLocation();
        }

        if (!peek()) {
            return {TokenType::PD_END, "", row, column};
        }

        const int tokenRow = row;
        const int tokenColumn = column;
        const char character = consumeAndTrackLocation();

        if (character == ';') {
            return {TokenType::PD_SEMI, ";", tokenRow, tokenColumn};
        }

        if (std::isalpha(static_cast<unsigned char>(character)) || character == '_') {
            std::string identifier(1, character);
            while (const auto next = peek()) {
                if (!std::isalnum(static_cast<unsigned char>(*next)) && *next != '_') {
                    break;
                }
                identifier += consumeAndTrackLocation();
            }
            const TokenType type = identifier == "return" ? TokenType::PD_RETURN
                                   : identifier == "exit" ? TokenType::PD_EXIT
                                                          : TokenType::PD_IDENTIFIER;
            return {type, identifier, tokenRow, tokenColumn};
        }

        if (std::isdigit(static_cast<unsigned char>(character))) {
            std::string number(1, character);
            while (const auto next = peek()) {
                if (!std::isdigit(static_cast<unsigned char>(*next))) {
                    break;
                }
                number += consumeAndTrackLocation();
            }

            bool isFloat = false;
            if (const auto next = peek(); next && *next == '.') {
                isFloat = true;
                number += consumeAndTrackLocation();
                while (const auto digit = peek()) {
                    if (!std::isdigit(static_cast<unsigned char>(*digit))) {
                        break;
                    }
                    number += consumeAndTrackLocation();
                }
            }

            const TokenType type =
                isFloat ? TokenType::PD_FLOAT_LITERAL : TokenType::PD_INT_LITERAL;
            return {type, number, tokenRow, tokenColumn};
        }

        return {TokenType::PD_UNDEFINED, std::string(1, character), tokenRow, tokenColumn};
    }

   private:
    [[nodiscard]] inline std::optional<char> peek(int ahead = 1) const {
        if (ahead < 1 || m_index + static_cast<std::size_t>(ahead) > m_src.size()) {
            return {};
        } else {
            return m_src.at(m_index + static_cast<std::size_t>(ahead - 1));
        }
    }

    inline char consume() { return m_src.at(m_index++); }
    std::string m_src;
    std::size_t m_index = 0;
    bool m_loaded = false;
};
