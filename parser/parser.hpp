#pragma once
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "../lexer/lexer.hpp"

struct NodeExpr;
struct NodeExprBinary {
    Token op;
    std::shared_ptr<NodeExpr> lhs;
    std::shared_ptr<NodeExpr> rhs;
};
struct NodeExpr {
    std::variant<Token, NodeExprBinary> var;
};
struct NodeStatExit {
    NodeExpr expr;
};
struct NodeStatPrint {
    NodeExpr expr;
};
struct NodeStat;
struct NodeStatLet {
    Token ident;
    NodeExpr expr;
};
struct NodeStatIf {
    NodeExpr condition;
    std::vector<NodeStat> body;
};
struct NodeStat {
    std::variant<NodeStatExit, NodeStatLet, NodeStatIf, NodeStatPrint> var;
};
struct NodeProg {
    std::vector<NodeStat> stats;
};

class Parser {
   public:
    explicit Parser(std::vector<Token> tokens, std::string filename = "<input>")
        : m_tokens(std::move(tokens)), m_filename(std::move(filename)) {}

    [[nodiscard]] std::optional<NodeExpr> parse_expr() { return parse_additive(); }

    std::optional<NodeStat> parse_stat() {
        const auto token = peek();
        if (token && token->type == TokenType::PD_EXIT) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after exit");
            }
            auto expr = parse_expr();
            if (!expr) {
                return error("Invalid exit expression");
            }
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after exit expression");
            }
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after exit statement");
            }
            return NodeStat{NodeStatExit{std::move(*expr)}};
        } else if (token && token->type == TokenType::PD_LET) {
            consume();
            const auto ident = peek();
            if (!ident || ident->type != TokenType::PD_IDENTIFIER) {
                return error("Expected identifier after let");
            }
            Token name = consume();
            if (!consume_if(TokenType::PD_EQUALS)) {
                return error("Expected '=' after identifier");
            }
            auto expr = parse_expr();
            if (!expr) {
                return error("Invalid let expression");
            }
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after let statement");
            }
            return NodeStat{NodeStatLet{std::move(name), std::move(*expr)}};
        } else if (token && token->type == TokenType::PD_IF) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after if");
            }
            auto condition = parse_expr();
            if (!condition) {
                return error("Expected condition in if statement");
            }
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after if condition");
            }
            if (!consume_if(TokenType::PD_OPEN_BRACE)) {
                return error("Expected '{' after if condition");
            }
            NodeStatIf if_stat{std::move(*condition), {}};
            while (peek() && peek()->type != TokenType::PD_CLOSE_BRACE) {
                if (peek()->type == TokenType::PD_END) {
                    return error("Expected '}' before end of input");
                }
                auto nested = parse_stat();
                if (!nested) {
                    return error("Invalid statement in if block");
                }
                if_stat.body.push_back(std::move(*nested));
            }
            if (!consume_if(TokenType::PD_CLOSE_BRACE)) {
                return error("Expected '}' to close if block");
            }
            return NodeStat{std::move(if_stat)};
        } else if (token && token->type == TokenType::PD_PRINT) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after print");
            }
            auto expr = parse_expr();
            if (!expr) {
                return error("Invalid print expression");
            }
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after print expression");
            }
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after print statement");
            }
            return NodeStat{NodeStatPrint{std::move(*expr)}};
        }
        return error("Expected a statement");
    }

    std::optional<NodeProg> parse_prog() {
        NodeProg prog;
        while (const auto token = peek()) {
            if (token->type == TokenType::PD_END) {
                consume();
                if (peek()) {
                    return error_prog("Unexpected tokens after end of input");
                }
                return prog;
            }
            auto stat = parse_stat();
            if (!stat) {
                return {};
            }
            prog.stats.push_back(std::move(*stat));
        }
        return prog;
    }

   private:
    std::optional<NodeExpr> parse_additive() {
        auto lhs = parse_multiplicative();
        if (!lhs) {
            return {};
        }
        while (peek() &&
               (peek()->type == TokenType::PD_PLUS || peek()->type == TokenType::PD_MINUS)) {
            Token op = consume();
            auto rhs = parse_multiplicative();
            if (!rhs) {
                return {};
            }
            lhs =
                NodeExpr{NodeExprBinary{std::move(op), std::make_shared<NodeExpr>(std::move(*lhs)),
                                        std::make_shared<NodeExpr>(std::move(*rhs))}};
        }
        return lhs;
    }
    std::optional<NodeExpr> parse_multiplicative() {
        auto lhs = parse_primary();
        if (!lhs) {
            return {};
        }
        while (peek() &&
               (peek()->type == TokenType::PD_STAR || peek()->type == TokenType::PD_SLASH)) {
            Token op = consume();
            auto rhs = parse_primary();
            if (!rhs) {
                return {};
            }
            lhs =
                NodeExpr{NodeExprBinary{std::move(op), std::make_shared<NodeExpr>(std::move(*lhs)),
                                        std::make_shared<NodeExpr>(std::move(*rhs))}};
        }
        return lhs;
    }
    std::optional<NodeExpr> parse_primary() {
        const auto token = peek();
        if (!token) {
            return {};
        }
        if (token->type == TokenType::PD_INT_LITERAL ||
            token->type == TokenType::PD_FLOAT_LITERAL ||
            token->type == TokenType::PD_STRING_LITERAL ||
            token->type == TokenType::PD_IDENTIFIER) {
            return NodeExpr{consume()};
        }
        if (token->type == TokenType::PD_OPEN_PAREN) {
            consume();
            auto expr = parse_expr();
            if (!expr || !consume_if(TokenType::PD_CLOSE_PAREN)) {
                return {};
            }
            return expr;
        }
        return {};
    }
    bool consume_if(TokenType type) {
        if (!peek() || peek()->type != type) {
            return false;
        }
        consume();
        return true;
    }
    std::optional<Token> peek() const {
        return m_index < m_tokens.size() ? std::optional<Token>(m_tokens[m_index]) : std::nullopt;
    }
    Token consume() { return m_tokens.at(m_index++); }
    std::optional<NodeStat> error(const char* message) const {
        reportError(m_filename, current_token(), message);
        return {};
    }
    std::optional<NodeProg> error_prog(const char* message) const {
        reportError(m_filename, current_token(), message);
        return {};
    }
    Token current_token() const {
        if (m_index < m_tokens.size()) return m_tokens[m_index];
        if (!m_tokens.empty()) return m_tokens.back();
        return Token{};
    }
    std::vector<Token> m_tokens;
    std::string m_filename;
    std::size_t m_index = 0;
};
