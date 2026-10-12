# PD Language Syntax

PD source files use the `.pd` extension. Statements end with a semicolon, except for an `if` block, which ends with `}`.

Comments can be written as `//` through the end of a line or as `/* ... */` across lines.

## Statements

```pd
let name = expression;
print(expression);
printnl(expression);
input(identifier);
exit(expression);

if (condition) {
    statement;
} else if (other_condition) {
    statement;
} else {
    statement;
}
```

- `let` declares a variable. Identifiers start with a letter or `_` and may contain letters, digits, and `_`. A name can only be declared once.
- `print` prints a numeric expression or a string without adding a newline. Strings can be joined with `+` and combined with numeric expressions or variables, for example `print("value: " + x);`.
- `printnl` prints a numeric expression or string, then adds a newline.
- `input` reads a number into an already declared variable, for example `input(age);`.
- `exit` ends the program and uses the integer conversion of its numeric expression as the exit code.
- `if` runs its body when its numeric condition is nonzero. `else if` checks another condition when earlier conditions are zero, and `else` runs when none of the preceding conditions are nonzero. Conditions are numeric expressions; comparisons are not currently supported.
- Statements can be nested inside `if` blocks.

## Expressions

```text
expression  ::= term (('+' | '-') term)*
term        ::= primary (('*' | '/') primary)*
primary     ::= number | identifier | string | '(' expression ')'
number      ::= integer | decimal
integer     ::= digit+
decimal     ::= digit+ '.' digit*
identifier  ::= (letter | '_') (letter | digit | '_')*
```

Multiplication and division have higher precedence than addition and subtraction. Operators at the same precedence are evaluated from left to right. Parentheses can group expressions. Numeric literals, including integers, are represented as floating-point values during arithmetic.

Strings use double quotes. The supported escapes are `\\n`, `\\t`, `\\"`, and `\\\\`. Strings are currently supported only in `print` expressions (including concatenation with `+`). Numeric parts in a mixed string expression are printed as numbers.

## Complete example

```pd
let x = 1.5;
let y = x + 2.25;
print(y);
print("result: " + "ready");
if (y) {
    print("y is nonzero");
}
exit(0);
```

`return` is recognized by the lexer but is not currently a supported statement.
