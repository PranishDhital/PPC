#pragma once
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "../lexer/lexer.hpp"

struct NodeExpr;
struct NodeStat;
struct NodeExprBinary {
    Token op;
    std::shared_ptr<NodeExpr> lhs;
    std::shared_ptr<NodeExpr> rhs;
};
struct NodeExprUnary {
    Token op;
    std::shared_ptr<NodeExpr> operand;
};
struct NodeExprIncrement {
    Token ident;
    bool prefix;
    bool decrement;
};
struct NodeExpr {
    std::variant<Token, NodeExprBinary, NodeExprUnary, NodeExprIncrement> var;
};
struct NodeStatExit {
    NodeExpr expr;
};
struct NodeStatPrint {
    NodeExpr expr;
};
struct NodeStatPrintnl {
    NodeExpr expr;
};
struct NodeStatExpr {
    NodeExpr expr;
};
struct NodeStatInput {
    Token ident;
};
struct NodeStatLet {
    Token ident;
    NodeExpr expr;
};
struct NodeStatIf {
    NodeExpr condition;
    std::vector<NodeStat> body;
    std::vector<NodeStat> else_body;
};

struct NodeStatWhile {
    NodeExpr condition;
    std::vector<NodeStat> body;
};
struct NodeStatFor {
    NodeStatLet initializer;
    NodeExpr condition;
    NodeExpr increment;
    std::vector<NodeStat> body;
};
struct NodeStat {
    std::variant<NodeStatExit, NodeStatLet, NodeStatIf, NodeStatPrint, NodeStatInput,
                 NodeStatPrintnl, NodeStatWhile, NodeStatExpr, NodeStatFor>
        var;
};
struct NodeProg {
    std::vector<NodeStat> stats;
};

class Parser {
   public:
    explicit Parser(std::vector<Token> tokens, std::string filename = "<input>")
        : m_tokens(std::move(tokens)), m_filename(std::move(filename)) {}

    [[nodiscard]] std::optional<NodeExpr> parse_expr() { return parse_comparison(); }

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
            NodeStatIf if_stat{std::move(*condition), {}, {}};
            if (!parse_block(if_stat.body, "if")) return {};
            if (consume_if(TokenType::PD_ELSE)) {
                if (peek() && peek()->type == TokenType::PD_IF) {
                    auto else_if = parse_stat();
                    if (!else_if) return {};
                    if_stat.else_body.push_back(std::move(*else_if));
                } else {
                    if (!consume_if(TokenType::PD_OPEN_BRACE)) {
                        return error("Expected 'if' or '{' after else");
                    }
                    if (!parse_block(if_stat.else_body, "else")) return {};
                }
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
        } else if (token && token->type == TokenType::PD_INPUT) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after input");
            }
            const auto ident = peek();
            if (!ident || ident->type != TokenType::PD_IDENTIFIER) {
                return error("Expected identifier in input statement");
            }
            Token name = consume();
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after input identifier");
            }
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after input statement");
            }
            return NodeStat{NodeStatInput{std::move(name)}};
        } else if (token && token->type == TokenType::PD_PRINT_NL) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after printnl");
            }
            auto expr = parse_expr();
            if (!expr) {
                return error("Invalid printnl expression");
            }
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after printnl expression");
            }
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after printnl statement");
            }
            return NodeStat{NodeStatPrintnl{std::move(*expr)}};
        } else if (token && token->type == TokenType::PD_WHILE) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after while");
            }
            auto condition = parse_expr();
            if (!condition) {
                return error("Invalid while expression");
            }
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after while expression");
            }
            if (!consume_if(TokenType::PD_OPEN_BRACE)) {
                return error("Expected '{' after condition in while loop");
            }
            // The while loop block
            NodeStatWhile while_stat{std::move(*condition), {}};
            if (!parse_block(while_stat.body, "while")) return {};
            return NodeStat{std::move(while_stat)};
        } else if (token && token->type == TokenType::PD_FOR) {
            consume();
            if (!consume_if(TokenType::PD_OPEN_PAREN)) {
                return error("Expected '(' after for");
            }
            if (!consume_if(TokenType::PD_LET)) {
                return error("Expected 'let' declaration in for initializer");
            }
            const auto ident = peek();
            if (!ident || ident->type != TokenType::PD_IDENTIFIER) {
                return error("Expected identifier in for initializer");
            }
            Token name = consume();
            if (!consume_if(TokenType::PD_EQUALS)) {
                return error("Expected '=' in for initializer");
            }
            auto initial_value = parse_expr();
            if (!initial_value) return error("Invalid for initializer expression");
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after for initializer");
            }
            auto condition = parse_expr();
            if (!condition) return error("Invalid for loop condition");
            if (!consume_if(TokenType::PD_SEMI)) {
                return error("Expected ';' after for loop condition");
            }
            auto increment = parse_expr();
            if (!increment) return error("Invalid for loop increment");
            if (!consume_if(TokenType::PD_CLOSE_PAREN)) {
                return error("Expected ')' after for clauses");
            }
            if (!consume_if(TokenType::PD_OPEN_BRACE)) {
                return error("Expected '{' after for clauses");
            }
            NodeStatFor for_stat{NodeStatLet{std::move(name), std::move(*initial_value)},
                                 std::move(*condition), std::move(*increment), {}};
            if (!parse_block(for_stat.body, "for")) return {};
            return NodeStat{std::move(for_stat)};
        }
        auto expr = parse_expr();
        if (!expr) return error("Expected a statement");
        if (!consume_if(TokenType::PD_SEMI)) return error("Expected ';' after expression");
        return NodeStat{NodeStatExpr{std::move(*expr)}};
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
    bool parse_block(std::vector<NodeStat>& body, const char* block_name) {
        while (peek() && peek()->type != TokenType::PD_CLOSE_BRACE) {
            if (peek()->type == TokenType::PD_END) {
                error("Expected '}' before end of input");
                return false;
            }
            auto nested = parse_stat();
            if (!nested) {
                error(block_name[0] == 'i' ? "Invalid statement in if block"
                       : block_name[0] == 'w' ? "Invalid statement in while block"
                       : block_name[0] == 'f' ? "Invalid statement in for block"
                                               : "Invalid statement in else block");
                return false;
            }
            body.push_back(std::move(*nested));
        }
        if (!consume_if(TokenType::PD_CLOSE_BRACE)) {
            error(block_name[0] == 'i' ? "Expected '}' to close if block"
                   : block_name[0] == 'w' ? "Expected '}' to close while block"
                   : block_name[0] == 'f' ? "Expected '}' to close for block"
                                           : "Expected '}' to close else block");
            return false;
        }
        return true;
    }
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
        auto lhs = parse_unary();
        if (!lhs) {
            return {};
        }
        while (peek() &&
               (peek()->type == TokenType::PD_STAR || peek()->type == TokenType::PD_SLASH)) {
            Token op = consume();
            auto rhs = parse_unary();
            if (!rhs) {
                return {};
            }
            lhs =
                NodeExpr{NodeExprBinary{std::move(op), std::make_shared<NodeExpr>(std::move(*lhs)),
                                        std::make_shared<NodeExpr>(std::move(*rhs))}};
        }
        return lhs;
    }
    std::optional<NodeExpr> parse_unary() {
        if (peek() && (peek()->type == TokenType::PD_PLUS ||
                       peek()->type == TokenType::PD_MINUS) &&
            peek(1) && peek(1)->type == peek()->type) {
            const bool decrement = peek()->type == TokenType::PD_MINUS;
            consume();
            consume();
            const auto ident = peek();
            if (!ident || ident->type != TokenType::PD_IDENTIFIER) return {};
            return NodeExpr{NodeExprIncrement{consume(), true, decrement}};
        }
        if (peek() && peek()->type == TokenType::PD_EXCLAIM) {
            Token op = consume();
            auto operand = parse_unary();
            if (!operand) return {};
            return NodeExpr{NodeExprUnary{std::move(op),
                                         std::make_shared<NodeExpr>(std::move(*operand))}};
        }
        return parse_primary();
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
            Token primary = consume();
            if (primary.type == TokenType::PD_IDENTIFIER && peek() &&
                (peek()->type == TokenType::PD_PLUS || peek()->type == TokenType::PD_MINUS) &&
                peek(1) && peek(1)->type == peek()->type) {
                const bool decrement = peek()->type == TokenType::PD_MINUS;
                consume();
                consume();
                return NodeExpr{NodeExprIncrement{std::move(primary), false, decrement}};
            }
            return NodeExpr{std::move(primary)};
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
    std::optional<NodeExpr> parse_comparison() {
        auto lhs = parse_additive();
        if (!lhs) {
            return {};
        }
        while (peek() && (peek()->type == TokenType::PD_GREATER_THAN ||
                          peek()->type == TokenType::PD_SMALLER_THAN ||
                          peek()->type == TokenType::PD_GREATER_EQUAL ||
                          peek()->type == TokenType::PD_SMALLER_EQUAL ||
                          peek()->type == TokenType::PD_DOUBLE_EQ ||
                          peek()->type == TokenType::PD_NOT_EQ)) {
            Token op = consume();
            auto rhs = parse_additive();
            if (!rhs) {
                return {};
            }
            lhs =
                NodeExpr{NodeExprBinary{std::move(op), std::make_shared<NodeExpr>(std::move(*lhs)),
                                        std::make_shared<NodeExpr>(std::move(*rhs))}};
        }
        return lhs;
    }
    bool consume_if(TokenType type) {
        if (!peek() || peek()->type != type) {
            return false;
        }
        consume();
        return true;
    }
    std::optional<Token> peek(std::size_t offset = 0) const {
        return m_index + offset < m_tokens.size()
                   ? std::optional<Token>(m_tokens[m_index + offset])
                   : std::nullopt;
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
