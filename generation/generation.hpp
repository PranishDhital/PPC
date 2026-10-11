#pragma once
#include <sstream>
#include <string>
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
            void operator()(const NodeExprIdent& Expr_Ident) {
                // TODO
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
                gen->m_output << "    ret\n";
            }
            void operator()(const NodeStatLet& stat_let) {}
        };

        StatVisitor visitor{.gen = this};
        std::visit(visitor, stat.var);
    }

    [[nodiscard]] inline std::string gen_prog() const {
        std::stringstream assembly;
        assembly << "global main\n";
        assembly << "section .text\n";
        assembly << "main:\n";
        for (const NodeStat& stat : m_prog.stats) {
            gen_stat(stat);
        }
        assembly << m_output.str();
        assembly << "    xor eax, eax\n";
        assembly << "    ret\n";
        return assembly.str();
    }

   private:
    const NodeProg m_prog;
    mutable std::stringstream m_output;
};
