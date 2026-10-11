#pragma once
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "../parser/parser.hpp"

class Generator {
   public:
    inline explicit Generator(NodeProg prog) : m_prog(std::move(prog)) {}

    void gen_expr(const NodeExpr& expr) const {
        struct ExprVisitor {
            const Generator* gen;
            void operator()(const NodeExprIntLit& Expr_Int_Lit) const {
                gen->m_output << "    mov eax, " << Expr_Int_Lit.int_lit.value << "\n";
            }
            void operator()(const NodeExprIdent& expr_ident) const {
                const auto it = gen->m_vars.find(expr_ident.Ident.value);
                if (it == gen->m_vars.end()) {
                    std::cerr << "Undefined identifier: " << expr_ident.Ident.value << '\n';
                    exit(EXIT_FAILURE);
                }
                gen->m_output << "    mov eax, [rbp - " << it->second.stack_loc * 8 << "]\n";
            }
        };
        ExprVisitor visitor{.gen = this};
        std::visit(visitor, expr.var);
    }

    void gen_stat(const NodeStat& stat) const {
        struct StatVisitor {
            const Generator* gen;
            void operator()(const NodeStatExit& stat_exit) {
                gen->gen_expr(stat_exit.expr);
                while (gen->m_stack_size > 1) {
                    gen->pop("rcx");
                }
                gen->pop("rbp");
                gen->m_output << "    ret\n";
            }
            void operator()(const NodeStatLet& stat_let) {
                if (gen->m_vars.contains(stat_let.ident.value)) {
                    std::cerr << "Identifier already used: " << stat_let.ident.value << '\n';
                    exit(EXIT_FAILURE);
                }
                gen->gen_expr(stat_let.expr);
                gen->push("rax");
                gen->m_vars.emplace(stat_let.ident.value, Var{gen->m_stack_size - 1});
            }
        };

        StatVisitor visitor{.gen = this};
        std::visit(visitor, stat.var);
    }

    [[nodiscard]] inline std::string gen_prog() const {
        std::stringstream assembly;
        assembly << "global main\n";
        assembly << "section .text\n";
        assembly << "main:\n";
        push("rbp");
        m_output << "    mov rbp, rsp\n";
        for (const NodeStat& stat : m_prog.stats) {
            gen_stat(stat);
        }
        assembly << m_output.str();
        assembly << "    xor eax, eax\n";
        assembly << "    ret\n";
        return assembly.str();
    }

   private:
    void push(const std::string& reg) const {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    void pop(const std::string& reg) const {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    struct Var {
        size_t stack_loc;
    };

    const NodeProg m_prog;
    mutable std::stringstream m_output;
    mutable size_t m_stack_size = 0;

    mutable std::unordered_map<std::string, Var> m_vars{};
};
