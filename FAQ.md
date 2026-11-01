# FAQ

## Why does not expr-core.c use tokenization->parsing->AST when parsing expressions?

The author is not familiar with tokenization and AST(may be the content of *the theory about compiling*), thus can only use a hard way in place. That is, to scan character-by-character and divide them into some unary and binary operations.

## Will it use tokenization->parsing->AST in parsing expressions in the future?

Maybe it will be refactored in the future, but the date will be not assured and not optimistic(the author has not studied the relevant theory and has no time to study it at the moment), depending on when the author has the ability to implement it.

## Where did the author see the terminology about lexer,tokenization,AST and the theory about compiling?

Recently(2026-10) from a GPT-like Chat after it received the code of libexpr.
Earlier, the author has only seen the word of the theory about compiling and believed that the theory was used for only writing the compiler(like gcc) and unrelated with parsing expressions.


