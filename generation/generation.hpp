#pragma once
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../parser/parser.hpp"

class Generator {
   public:
    explicit Generator(NodeProg prog, std::string filename = "<input>")
        : m_prog(std::move(prog)), m_filename(std::move(filename)) {}

    [[nodiscard]] std::string gen_prog() const {
        m_output << "main:\n    push rbp\n    mov rbp, rsp\n";
        for (const auto& stat : m_prog.stats) gen_stat(stat);
        m_output << "    xor eax, eax\n    mov rsp, rbp\n    pop rbp\n    ret\n";
        std::stringstream assembly;
        assembly << "default rel\nextern printf\nextern scanf\nglobal main\nsection .data\n"
                    "format_double: db '%g', 0\nformat_string: db '%s', 0\n"
                    "format_input_double: db '%lf', 0\n"
                    "format_input_string: db ' %255[^', 10, ']', 0\n"
                    "format_double_inline: db '%g', 0\nformat_string_inline: db '%s', 0\n"
                    "newline: db 10, 0\n";
        for (const auto& [label, value] : m_string_constants) {
            assembly << label << ": db ";
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (i != 0) assembly << ", ";
                assembly << static_cast<unsigned int>(static_cast<unsigned char>(value[i]));
            }
            if (!value.empty()) assembly << ", ";
            assembly << "0\n";
        }
        assembly << "section .text\n" << m_output.str();
        return assembly.str();
    }

   private:
    void gen_expr(const NodeExpr& expr) const {
        if (const auto* token = std::get_if<Token>(&expr.var)) {
            if (token->type == TokenType::PD_STRING_LITERAL) {
                reportError(m_filename, *token, "String values are only supported by print");
                std::exit(EXIT_FAILURE);
            } else if (token->type == TokenType::PD_INT_LITERAL ||
                       token->type == TokenType::PD_FLOAT_LITERAL) {
                const double value = std::stod(token->value);
                const auto bits = std::bit_cast<std::uint64_t>(value);
                m_output << "    mov rax, " << bits << "\n    movq xmm0, rax\n";
            } else {
                const auto it = m_vars.find(token->value);
                if (it == m_vars.end()) {
                    reportError(m_filename, *token, "Undefined identifier: " + token->value);
                    std::exit(EXIT_FAILURE);
                }
                m_output << "    movsd xmm0, [rbp - " << it->second << "]\n";
            }
            return;
        }
        if (const auto* unary = std::get_if<NodeExprUnary>(&expr.var)) {
            gen_expr(*unary->operand);
            if (unary->op.type == TokenType::PD_EXCLAIM) {
                m_output << "    xorpd xmm1, xmm1\n    ucomisd xmm0, xmm1\n"
                            "    sete al\n    movzx eax, al\n    cvtsi2sd xmm0, eax\n";
            } else if (unary->op.type == TokenType::PD_MINUS) {
                m_output << "    xorpd xmm1, xmm1\n    subsd xmm1, xmm0\n    movapd xmm0, xmm1\n";
            } else if (unary->op.type != TokenType::PD_PLUS) {
                reportError(m_filename, unary->op, "Unsupported unary operator");
                std::exit(EXIT_FAILURE);
            }
            return;
        }
        if (const auto* increment = std::get_if<NodeExprIncrement>(&expr.var)) {
            const auto it = m_vars.find(increment->ident.value);
            if (it == m_vars.end()) {
                reportError(m_filename, increment->ident,
                            "Undefined identifier: " + increment->ident.value);
                std::exit(EXIT_FAILURE);
            }
            m_output << "    movsd xmm0, [rbp - " << it->second << "]\n";
            if (!increment->prefix) m_output << "    movapd xmm1, xmm0\n";
            m_output << "    mov rax, 4607182418800017408\n    movq xmm2, rax\n    "
                     << (increment->decrement ? "subsd" : "addsd")
                     << " xmm0, xmm2\n    movsd [rbp - "
                     << it->second << "], xmm0\n";
            if (!increment->prefix) m_output << "    movapd xmm0, xmm1\n";
            return;
        }
        const auto& binary = std::get<NodeExprBinary>(expr.var);
        gen_expr(*binary.lhs);
        m_output << "    sub rsp, 8\n    movsd [rsp], xmm0\n";
        gen_expr(*binary.rhs);
        m_output << "    movapd xmm1, xmm0\n    movsd xmm0, [rsp]\n    add rsp, 8\n";
        switch (binary.op.type) {
            case TokenType::PD_SMALLER_THAN:
            case TokenType::PD_SMALLER_EQUAL:
            case TokenType::PD_GREATER_THAN:
            case TokenType::PD_GREATER_EQUAL:
            case TokenType::PD_DOUBLE_EQ:
            case TokenType::PD_NOT_EQ:
                m_output << "    ucomisd xmm0, xmm1\n";
                switch (binary.op.type) {
                    case TokenType::PD_SMALLER_THAN: m_output << "    setb al\n"; break;
                    case TokenType::PD_GREATER_THAN: m_output << "    seta al\n"; break;
                    case TokenType::PD_GREATER_EQUAL: m_output << "    setae al\n"; break;
                    case TokenType::PD_SMALLER_EQUAL:
                        m_output << "    setbe al\n    setnp dl\n    and al, dl\n";
                        break;
                    case TokenType::PD_DOUBLE_EQ:
                        m_output << "    sete al\n    setnp dl\n    and al, dl\n";
                        break;
                    case TokenType::PD_NOT_EQ:
                        m_output << "    setne al\n    setp dl\n    or al, dl\n";
                        break;
                    default: break;
                }
                m_output << "    movzx eax, al\n    cvtsi2sd xmm0, eax\n";
                return;
            default: break;
        }
        switch (binary.op.type) {
            case TokenType::PD_PLUS:
                m_output << "    addsd xmm0, xmm1\n";
                break;
            case TokenType::PD_MINUS:
                m_output << "    subsd xmm0, xmm1\n";
                break;
            case TokenType::PD_STAR:
                m_output << "    mulsd xmm0, xmm1\n";
                break;
            case TokenType::PD_SLASH:
                m_output << "    divsd xmm0, xmm1\n";
                break;
            default:
                reportError(m_filename, binary.op, "Unsupported arithmetic operator");
                std::exit(EXIT_FAILURE);
        }
    }
    void gen_stat(const NodeStat& stat) const {
        if (const auto* exit = std::get_if<NodeStatExit>(&stat.var)) {
            gen_expr(exit->expr);
            m_output << "    cvttsd2si eax, xmm0\n    mov rsp, rbp\n    pop rbp\n    ret\n";
        } else if (const auto* let = std::get_if<NodeStatLet>(&stat.var)) {
            if (m_vars.contains(let->ident.value) || m_string_vars.contains(let->ident.value)) {
                reportError(m_filename, let->ident, "Identifier already used: " + let->ident.value);
                std::exit(EXIT_FAILURE);
            }
            gen_expr(let->expr);
            m_stack_slots++;
            m_output << "    sub rsp, 8\n    movsd [rbp - " << m_stack_slots * 8 << "], xmm0\n";
            m_vars.emplace(let->ident.value, m_stack_slots * 8);
        } else if (const auto* string = std::get_if<NodeStatString>(&stat.var)) {
            if (m_vars.contains(string->ident.value) ||
                m_string_vars.contains(string->ident.value)) {
                reportError(m_filename, string->ident,
                            "Identifier already used: " + string->ident.value);
                std::exit(EXIT_FAILURE);
            }
            const auto value = eval_string(string->expr);
            if (!value) {
                reportError(m_filename, string->ident,
                            "String declarations require a string literal expression");
                std::exit(EXIT_FAILURE);
            }
            const std::string label = string_label(*value);
            m_stack_slots++;
            m_output << "    sub rsp, 8\n    lea rax, [rel " << label << "]\n    mov [rbp - "
                     << m_stack_slots * 8 << "], rax\n";
            m_string_vars.emplace(string->ident.value, m_stack_slots * 8);
        } else if (const auto* input = std::get_if<NodeStatInput>(&stat.var)) {
            if (const auto string_it = m_string_vars.find(input->ident.value);
                string_it != m_string_vars.end()) {
                m_stack_slots += 32;
                const std::size_t buffer_offset = m_stack_slots * 8;
                const std::size_t call_space = (m_stack_slots % 2 == 0) ? 32 : 40;
                m_output << "    sub rsp, 256\n    lea rax, [rbp - " << buffer_offset
                         << "]\n    mov [rbp - " << string_it->second
                         << "], rax\n    mov rdx, rax\n"
                            "    lea rcx, [rel format_input_string]\n    sub rsp, "
                         << call_space << "\n    call scanf\n    add rsp, " << call_space << '\n';
                return;
            }
            const auto it = m_vars.find(input->ident.value);
            if (it == m_vars.end()) {
                reportError(m_filename, input->ident,
                            "Undefined identifier: " + input->ident.value);
                std::exit(EXIT_FAILURE);
            }
            const std::size_t call_space = (m_stack_slots % 2 == 0) ? 32 : 40;
            m_output << "    lea rdx, [rbp - " << it->second
                     << "]\n"
                        "    lea rcx, [rel format_input_double]\n    sub rsp, "
                     << call_space << "\n    call scanf\n    add rsp, " << call_space << '\n';
        } else if (const auto* print = std::get_if<NodeStatPrint>(&stat.var)) {
            const std::size_t call_space = (m_stack_slots % 2 == 0) ? 32 : 40;
            if (contains_string(print->expr)) {
                std::vector<const NodeExpr*> parts;
                flatten_concat(print->expr, parts);
                for (const NodeExpr* part : parts) {
                    if (const auto* token = std::get_if<Token>(&part->var);
                        token && token->type == TokenType::PD_STRING_LITERAL) {
                        const std::string label = string_label(token->value);
                        m_output << "    lea rdx, [rel " << label
                                 << "]\n"
                                    "    lea rcx, [rel format_string_inline]\n    sub rsp, "
                                 << call_space << "\n    call printf\n    add rsp, " << call_space
                                 << '\n';
                    } else if (token && m_string_vars.contains(token->value)) {
                        m_output << "    mov rdx, [rbp - " << m_string_vars.at(token->value)
                                 << "]\n    lea rcx, [rel format_string_inline]\n    sub rsp, "
                                 << call_space << "\n    call printf\n    add rsp, " << call_space
                                 << '\n';
                    } else {
                        gen_expr(*part);
                        m_output << "    movq rdx, xmm0\n"
                                    "    lea rcx, [rel format_double_inline]\n    sub rsp, "
                                 << call_space << "\n    call printf\n    add rsp, " << call_space
                                 << '\n';
                    }
                }
            } else {
                gen_expr(print->expr);
                m_output << "    movapd xmm1, xmm0\n    movq rdx, xmm0\n"
                            "    lea rcx, [rel format_double]\n    sub rsp, "
                         << call_space << "\n    call printf\n    add rsp, " << call_space << '\n';
            }
        } else if (const auto* printnl = std::get_if<NodeStatPrintnl>(&stat.var)) {
            const std::size_t call_space = (m_stack_slots % 2 == 0) ? 32 : 40;
            if (contains_string(printnl->expr)) {
                std::vector<const NodeExpr*> parts;
                flatten_concat(printnl->expr, parts);
                for (const NodeExpr* part : parts) {
                    if (const auto* token = std::get_if<Token>(&part->var);
                        token && token->type == TokenType::PD_STRING_LITERAL) {
                        const std::string label = string_label(token->value);
                        m_output << "    lea rdx, [rel " << label
                                 << "]\n"
                                    "    lea rcx, [rel format_string_inline]\n    sub rsp, "
                                 << call_space << "\n    call printf\n    add rsp, " << call_space
                                 << '\n';
                    } else if (token && m_string_vars.contains(token->value)) {
                        m_output << "    mov rdx, [rbp - " << m_string_vars.at(token->value)
                                 << "]\n    lea rcx, [rel format_string_inline]\n    sub rsp, "
                                 << call_space << "\n    call printf\n    add rsp, " << call_space
                                 << '\n';
                    } else {
                        gen_expr(*part);
                        m_output << "    movq rdx, xmm0\n"
                                    "    lea rcx, [rel format_double_inline]\n    sub rsp, "
                                 << call_space << "\n    call printf\n    add rsp, " << call_space
                                 << '\n';
                    }
                }
            } else {
                gen_expr(printnl->expr);
                m_output << "    movapd xmm1, xmm0\n    movq rdx, xmm0\n"
                            "    lea rcx, [rel format_double]\n    sub rsp, "
                         << call_space << "\n    call printf\n    add rsp, " << call_space << '\n';
            }
            m_output << "    lea rdx, [rel newline]\n"
                        "    lea rcx, [rel format_string]\n    sub rsp, "
                     << call_space << "\n    call printf\n    add rsp, " << call_space << '\n';
        } else if (const auto* expr = std::get_if<NodeStatExpr>(&stat.var)) {
            gen_expr(expr->expr);
        } else if (const auto* if_stat = std::get_if<NodeStatIf>(&stat.var)) {
            const std::size_t label = m_next_label++;
            gen_expr(if_stat->condition);
            m_output << "    xorpd xmm1, xmm1\n    ucomisd xmm0, xmm1\n    je .if_end_" << label
                     << '\n';
            for (const auto& nested : if_stat->body) gen_stat(nested);
            if (!if_stat->else_body.empty()) {
                m_output << "    jmp .if_done_" << label << '\n';
            }
            m_output << ".if_end_" << label << ":\n";
            for (const auto& nested : if_stat->else_body) gen_stat(nested);
            if (!if_stat->else_body.empty()) m_output << ".if_done_" << label << ":\n";
        } else if (const auto* while_stat = std::get_if<NodeStatWhile>(&stat.var)) {
            const std::size_t label = m_next_label++;
            m_output << ".while_start_" << label << ":\n";
            gen_expr(while_stat->condition);
            m_output << "    xorpd xmm1, xmm1\n    ucomisd xmm0, xmm1\n    je .while_end_"
                     << label << '\n';
            for (const auto& nested : while_stat->body) gen_stat(nested);
            m_output << "    jmp .while_start_" << label << "\n.while_end_" << label << ":\n";
        } else {
            const auto& for_stat = std::get<NodeStatFor>(stat.var);
            gen_stat(NodeStat{for_stat.initializer});
            const std::size_t label = m_next_label++;
            m_output << ".for_start_" << label << ":\n";
            gen_expr(for_stat.condition);
            m_output << "    xorpd xmm1, xmm1\n    ucomisd xmm0, xmm1\n    je .for_end_"
                     << label << '\n';
            for (const auto& nested : for_stat.body) gen_stat(nested);
            gen_expr(for_stat.increment);
            m_output << "    jmp .for_start_" << label << "\n.for_end_" << label << ":\n";
        }
    }
    std::optional<std::string> eval_string(const NodeExpr& expr) const {
        if (const auto* token = std::get_if<Token>(&expr.var)) {
            if (token->type == TokenType::PD_STRING_LITERAL) return token->value;
            return {};
        }
        if (std::holds_alternative<NodeExprUnary>(expr.var) ||
            std::holds_alternative<NodeExprIncrement>(expr.var)) return {};
        const auto& binary = std::get<NodeExprBinary>(expr.var);
        if (binary.op.type != TokenType::PD_PLUS) return {};
        auto lhs = eval_string(*binary.lhs);
        auto rhs = eval_string(*binary.rhs);
        if (!lhs || !rhs) return {};
        return *lhs + *rhs;
    }
    bool contains_string(const NodeExpr& expr) const {
        if (const auto* token = std::get_if<Token>(&expr.var)) {
            return token->type == TokenType::PD_STRING_LITERAL ||
                   m_string_vars.contains(token->value);
        }
        if (const auto* unary = std::get_if<NodeExprUnary>(&expr.var)) {
            return contains_string(*unary->operand);
        }
        if (std::holds_alternative<NodeExprIncrement>(expr.var)) return false;
        const auto& binary = std::get<NodeExprBinary>(expr.var);
        return binary.op.type == TokenType::PD_PLUS &&
               (contains_string(*binary.lhs) || contains_string(*binary.rhs));
    }
    void flatten_concat(const NodeExpr& expr, std::vector<const NodeExpr*>& parts) const {
        if (const auto* binary = std::get_if<NodeExprBinary>(&expr.var);
            binary && binary->op.type == TokenType::PD_PLUS &&
            (contains_string(*binary->lhs) || contains_string(*binary->rhs))) {
            flatten_concat(*binary->lhs, parts);
            flatten_concat(*binary->rhs, parts);
        } else {
            parts.push_back(&expr);
        }
    }
    std::string string_label(const std::string& value) const {
        const auto found = m_string_labels.find(value);
        if (found != m_string_labels.end()) return found->second;
        const std::string label = "string_" + std::to_string(m_string_constants.size());
        m_string_labels.emplace(value, label);
        m_string_constants.emplace_back(label, value);
        return label;
    }
    const NodeProg m_prog;
    const std::string m_filename;
    mutable std::stringstream m_output;
    mutable std::size_t m_stack_slots = 0;
    mutable std::size_t m_next_label = 0;
    mutable std::unordered_map<std::string, std::size_t> m_vars;
    mutable std::unordered_map<std::string, std::size_t> m_string_vars;
    mutable std::unordered_map<std::string, std::string> m_string_labels;
    mutable std::vector<std::pair<std::string, std::string>> m_string_constants;
};
