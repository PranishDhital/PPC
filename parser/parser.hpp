#pragma once
#include <cstddef>
#include <iostream>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "../lexer/lexer.hpp"

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token Ident;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeStatExit {
    NodeExpr expr;
};

struct NodeStatLet {
    Token ident;
    NodeExpr expr;
};
struct NodeStat {
    std::variant<NodeStatExit, NodeStatLet> var;
};

struct NodeProg {
    std::vector<NodeStat> stats;
};

class Parser {
   public:
    /**
     * @brief Creates a parser that takes ownership of a token sequence.
     * @param tokens Tokens to parse, including the final PD_END token when available.
     */
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)) {}

    /**
     * @brief Parses an integer literal expression at the current token position.
     * @return The parsed expression and advances past its token, or an empty optional
     *         when the next token is not an integer literal.
     */
    [[nodiscard]] inline std::optional<NodeExpr> parse_expr() {
        const auto token = peek();
        if (token && token->type == TokenType::PD_INT_LITERAL) {
            return NodeExpr{NodeExprIntLit{consume()}};
        } else if (token && token->type == TokenType::PD_IDENTIFIER) {
            return NodeExpr{NodeExprIdent{consume()}};
        }
        return {};
    }

    std::optional<NodeStat> parse_stat() {
        const auto token = peek();
        if (token && token->type == TokenType::PD_EXIT) {
            consume();
            const auto open_paren = peek();
            if (!open_paren || open_paren->type != TokenType::PD_OPEN_PAREN) {
                std::cerr << "Expected '(' after exit\n";
                return {};
            }
            consume();

            auto node_expr = parse_expr();
            if (!node_expr) {
                std::cerr << "Invalid expression\n";
                return {};
            }

            const auto close_paren = peek();
            if (!close_paren || close_paren->type != TokenType::PD_CLOSE_PAREN) {
                std::cerr << "Expected ')' after exit expression\n";
                return {};
            }
            consume();

            const auto semicolon = peek();
            if (!semicolon || semicolon->type != TokenType::PD_SEMI) {
                std::cerr << "Expected ';' after exit expression\n";
                return {};
            }
            consume();

            return NodeStat{NodeStatExit{std::move(*node_expr)}};
        } else if (token && token->type == TokenType::PD_LET &&
                   peek(2).has_value() && peek(2)->type == TokenType::PD_IDENTIFIER &&
                   peek(3).has_value() && peek(3)->type == TokenType::PD_EQUALS) {
            consume();
            auto stat_let = NodeStatLet{.ident = consume()};
            consume();

            if (auto expr = parse_expr()) {
                stat_let.expr = std::move(*expr);
            } else {
                std::cerr << "Invalid Expression\n";
                return {};
            }
            const auto semicolon = peek();
            if (!semicolon || semicolon->type != TokenType::PD_SEMI) {
                std::cerr << "Expected an ';' at the end of a statement\n";
                return {};
            }
            consume();
            return NodeStat{.var = stat_let};
        } else {
            return {};
        }
    }

    std::optional<NodeProg> parse_prog() {
        NodeProg prog;
        while (const auto token = peek()) {
            if (token->type == TokenType::PD_END) {
                consume();
                if (peek()) {
                    std::cerr << "Unexpected tokens after end of input\n";
                    return {};
                }
                return prog;
            }
            if (auto stat = parse_stat()) {
                prog.stats.push_back(std::move(*stat));
            } else {
                std::cerr << "Invalid statement\n";
                return {};
            }
        }
        return prog;
    }

   private:
    /**
     * @brief Looks ahead in the token sequence without consuming a token.
     * @param offset One-based token offset from the current position.
     * @return The token at that offset, or an empty optional if it is out of range.
     */
    [[nodiscard]] inline std::optional<Token> peek(std::size_t offset = 1) const {
        if (offset == 0 || offset > m_tokens.size() - m_index) {
            return {};
        } else {
            return m_tokens.at(m_index + offset - 1);
        }
    }

    /**
     * @brief Returns and consumes the next token.
     * @return The consumed token.
     * @pre The current token position is within the token sequence.
     */
    inline Token consume() { return m_tokens.at(m_index++); }
    const std::vector<Token> m_tokens;
    std::size_t m_index = 0;
};
