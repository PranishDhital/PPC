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
    PD_STRING_LITERAL,
    PD_END,
    PD_OPEN_PAREN,
    PD_CLOSE_PAREN,
    PD_LET,
    PD_EQUALS,
    PD_PLUS,
    PD_MINUS,
    PD_STAR,
    PD_SLASH,
    PD_PRINT,
    PD_PRINT_NL,
    PD_IF,
    PD_ELSE,
    PD_INPUT,
    PD_OPEN_BRACE,
    PD_CLOSE_BRACE,
    PD_SMALLER_THAN,
    PD_GREATER_THAN,
    PD_SMALLER_THAN_AND_EQ,
    PD_GREATER_THAN_AND_EQ,
    PD_EXCLAIM,
    PD_UNDEFINED,
};

struct Token {
    TokenType type = TokenType::PD_UNDEFINED;
    std::string value;
    int row = 1;
    int column = 1;
};

inline void reportError(const std::string& filename, const Token& token,
                        const std::string& message) {
    std::cerr << "In " << filename << " at " << token.row << ':' << token.column
              << ": error: " << message << '\n';
}

class lexer {
   public:
    /**
     * @brief Prints a token's source location, type, and value.
     * @param token Token to print. End-of-input tokens are omitted.
     */
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
            case TokenType::PD_EQUALS:
                name = "EQUALS";
                break;
            case TokenType::PD_PLUS:
                name = "PLUS";
                break;
            case TokenType::PD_MINUS:
                name = "MINUS";
                break;
            case TokenType::PD_STAR:
                name = "STAR";
                break;
            case TokenType::PD_SLASH:
                name = "SLASH";
                break;
            case TokenType::PD_IF:
                name = "IF";
                break;
            case TokenType::PD_OPEN_BRACE:
                name = "OPEN_BRACE";
                break;
            case TokenType::PD_CLOSE_BRACE:
                name = "CLOSE_BRACE";
                break;
            case TokenType::PD_OPEN_PAREN:
                name = "OPEN_PAREN";
                break;
            case TokenType::PD_CLOSE_PAREN:
                name = "CLOSE_PAREN";
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
            case TokenType::PD_STRING_LITERAL:
                name = "STRING_LITERAL";
                break;
            case TokenType::PD_LET:
                name = "LET";
                break;
            case TokenType::PD_PRINT:
                name = "PRINT";
                break;
            case TokenType::PD_INPUT:
                name = "INPUT";
                break;
            case TokenType::PD_PRINT_NL:
                name = "PRINTNL";
                break;
            case TokenType::PD_ELSE:
                name = "ELSE";
                break;
            case TokenType::PD_SMALLER_THAN:
                name = "SMALLER_THAN";
                break;
            case TokenType::PD_GREATER_THAN:
                name = "GREATER_THAN";
                break;
            case TokenType::PD_SMALLER_THAN_AND_EQ:
                name = "SMALLER_THAN_EQ";
                break;
            case TokenType::PD_GREATER_THAN_AND_EQ:
                name = "GREATER_THAN_EQ";
                break;
            case TokenType::PD_EXCLAIM:
                name = "EXCLAMATION";
                break;
            case TokenType::PD_UNDEFINED:
                break;
            case TokenType::PD_END:
                return;
        }
        std::cout << token.row << ':' << token.column << "  " << name << " : ' " << token.value
                  << " '\n";
    }

    /**
     * @brief Reads and returns the next token from the input stream.
     * @param file Source stream; its contents are loaded the first time this is called.
     * @param row Current one-based source row, updated as characters are consumed.
     * @param column Current one-based source column, updated as characters are consumed.
     * @return The next token, or a PD_END token when the source is exhausted.
     */
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

        while (true) {
            while (const auto character = peek()) {
                if (!std::isspace(static_cast<unsigned char>(*character))) break;
                consumeAndTrackLocation();
            }

            const auto commentType = peek(2);
            if (peek() != '/' || !commentType || (*commentType != '/' && *commentType != '*')) {
                break;
            }
            const int commentRow = row;
            const int commentColumn = column;
            consumeAndTrackLocation();
            consumeAndTrackLocation();

            if (*commentType == '/') {
                while (peek()) {
                    if (consumeAndTrackLocation() == '\n') break;
                }
                continue;
            }

            bool closed = false;
            while (peek()) {
                const char current = consumeAndTrackLocation();
                if (current == '*' && peek() == '/') {
                    consumeAndTrackLocation();
                    closed = true;
                    break;
                }
            }
            if (!closed) {
                return {TokenType::PD_UNDEFINED, "Unterminated block comment", commentRow,
                        commentColumn};
            }
        }

        if (!peek()) {
            return {TokenType::PD_END, "", row, column};
        }

        const int tokenRow = row;
        const int tokenColumn = column;
        const char character = consumeAndTrackLocation();

        if (character == ';') {
            return {TokenType::PD_SEMI, ";", tokenRow, tokenColumn};
        } else if (character == '(') {
            return {TokenType::PD_OPEN_PAREN, "(", tokenRow, tokenColumn};
        } else if (character == ')') {
            return {TokenType::PD_CLOSE_PAREN, ")", tokenRow, tokenColumn};
        } else if (character == '+') {
            return {TokenType::PD_PLUS, "+", tokenRow, tokenColumn};
        } else if (character == '-') {
            return {TokenType::PD_MINUS, "-", tokenRow, tokenColumn};
        } else if (character == '*') {
            return {TokenType::PD_STAR, "*", tokenRow, tokenColumn};
        } else if (character == '/') {
            return {TokenType::PD_SLASH, "/", tokenRow, tokenColumn};
        } else if (character == '{') {
            return {TokenType::PD_OPEN_BRACE, "{", tokenRow, tokenColumn};
        } else if (character == '}') {
            return {TokenType::PD_CLOSE_BRACE, "}", tokenRow, tokenColumn};
        } else if (character == '<') {
            if (peek() == '=') {
                consumeAndTrackLocation();
                return {TokenType::PD_SMALLER_THAN_AND_EQ, "<=", tokenRow, tokenColumn};
            }
            return {TokenType::PD_SMALLER_THAN, "<", tokenRow, tokenColumn};
        } else if (character == '>') {
            if (peek() == '=') {
                consumeAndTrackLocation();
                return {TokenType::PD_GREATER_THAN_AND_EQ, ">=", tokenRow, tokenColumn};
            }
            return {TokenType::PD_GREATER_THAN, ">", tokenRow, tokenColumn};
        } else if (character == '=') {
            return {TokenType::PD_EQUALS, "=", tokenRow, tokenColumn};
        } else if (character == '!') {
            return {TokenType::PD_EXCLAIM, "!", tokenRow, tokenColumn};
        }

        if (std::isalpha(static_cast<unsigned char>(character)) || character == '_') {
            std::string identifier(1, character);
            while (const auto next = peek()) {
                if (!std::isalnum(static_cast<unsigned char>(*next)) && *next != '_') {
                    break;
                }
                identifier += consumeAndTrackLocation();
            }
            const TokenType type = identifier == "return"    ? TokenType::PD_RETURN
                                   : identifier == "exit"    ? TokenType::PD_EXIT
                                   : identifier == "let"     ? TokenType::PD_LET
                                   : identifier == "print"   ? TokenType::PD_PRINT
                                   : identifier == "if"      ? TokenType::PD_IF
                                   : identifier == "input"   ? TokenType::PD_INPUT
                                   : identifier == "printnl" ? TokenType::PD_PRINT_NL
                                   : identifier == "else"    ? TokenType::PD_ELSE
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

        if (character == '"') {
            std::string value;
            while (const auto next = peek()) {
                const char current = consumeAndTrackLocation();
                if (current == '"') {
                    return {TokenType::PD_STRING_LITERAL, value, tokenRow, tokenColumn};
                }
                if (current == '\\') {
                    const auto escaped = peek();
                    if (!escaped) break;
                    const char escapeChar = consumeAndTrackLocation();
                    switch (escapeChar) {
                        case 'n':
                            value += '\n';
                            break;
                        case 't':
                            value += '\t';
                            break;
                        case '"':
                            value += '"';
                            break;
                        case '\\':
                            value += '\\';
                            break;
                        default:
                            return {TokenType::PD_UNDEFINED, std::string(1, escapeChar), tokenRow,
                                    tokenColumn};
                    }
                } else {
                    value += current;
                }
            }
            return {TokenType::PD_UNDEFINED, "Unterminated string", tokenRow, tokenColumn};
        }

        return {TokenType::PD_UNDEFINED, std::string(1, character), tokenRow, tokenColumn};
    }

   private:
    /**
     * @brief Looks ahead in the loaded source without consuming characters.
     * @param offset One-based character offset from the current position.
     * @return The character at the requested offset, or an empty optional past end-of-input.
     */
    [[nodiscard]] inline std::optional<char> peek(int offset = 1) const {
        if (offset < 1 || m_index + static_cast<std::size_t>(offset) > m_src.size()) {
            return {};
        } else {
            return m_src.at(m_index + static_cast<std::size_t>(offset - 1));
        }
    }

    /**
     * @brief Returns and consumes the next character in the loaded source.
     * @return The consumed character.
     * @pre The current source position is before the end of the source.
     */
    inline char consume() { return m_src.at(m_index++); }
    std::string m_src;
    std::size_t m_index = 0;
    bool m_loaded = false;
};
