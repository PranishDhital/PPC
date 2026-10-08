#pragma once
#include <istream>
#include <string>

enum class TokenType {
    PD_RETURN,
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
    void print(const Token& token);
    Token getNextToken(std::istream& file, int& row, int& column);

   private:
};
