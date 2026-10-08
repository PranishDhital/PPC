#pragma once
#include <string>
#include <utility>

#include "../parser/parser.hpp"

class Generator {
   public:
    inline explicit Generator(NodeExit root) : m_root(std::move(root)) {}

    [[nodiscard]] inline std::string generate() const {
        std::string assembly =
            "global main\n"
            "section .text\n"
            "main:\n"
            "    mov eax, " +
            m_root.expr.int_lit.value +
            "\n"
            "    ret\n";
        return assembly;
    }

   private:
    const NodeExit m_root;
};
