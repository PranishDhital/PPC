#pragma once
#include <cstddef>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

#include "../lexer/lexer.hpp"

struct NodeExpr {
    Token int_lit;
};
struct NodeExit {
    NodeExpr expr;
};

class Parser {
   public:
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

    [[nodiscard]] inline std::optional<NodeExpr> parse_expr() {
        const auto token = peek();
        if (token && token->type == TokenType::PD_INT_LITERAL) {
            return NodeExpr{consume()};
        }
        return {};
    }

    [[nodiscard]] inline std::optional<NodeExit> parse() {
        const auto token = peek();
        if (!token || token->type != TokenType::PD_EXIT) {
            return {};
        }

        consume();
        auto node_expr = parse_expr();
        if (!node_expr) {
            std::cerr << "Invalid expression\n";
            return {};
        }

        const auto semicolon = peek();
        if (!semicolon || semicolon->type != TokenType::PD_SEMI) {
            std::cerr << "Expected ';' after exit expression\n";
            return {};
        }
        consume();

        if (const auto end = peek(); end && end->type == TokenType::PD_END) {
            consume();
        } else if (peek()) {
            std::cerr << "Unexpected token after exit statement\n";
            return {};
        }

        return NodeExit{std::move(*node_expr)};
    }

   private:
    [[nodiscard]] inline std::optional<Token> peek(std::size_t ahead = 1) const {
        if (ahead == 0 || ahead > m_tokens.size() - m_index) {
            return {};
        } else {
            return m_tokens.at(m_index + ahead - 1);
        }
    }

    inline Token consume() { return m_tokens.at(m_index++); }
    const std::vector<Token> m_tokens;
    std::size_t m_index = 0;
};
