# FAQ

## Why does not expr-core.c use tokenization->parsing->AST when parsing expressions?

The author is not familiar with tokenization and AST(may be the content of *the theory about compiling*), thus can only use a hard way in place. That is, to scan character-by-character and divide them into some unary and binary operations.

## Will it use tokenization->parsing->AST in parsing expressions in the future?

Maybe it will be refactored in the future, but the date will be not assured and not optimistic(the author has not studied the relevant theory and has no time to study it at the moment), depending on when the author has the ability to implement it.

## Where did the author see the terminology about lexer,tokenization,AST and the theory about compiling?

Recently(2026-10) from a GPT-like Chat after it received the code of libexpr.
Earlier, the author has only seen the word of the theory about compiling and believed that the theory was only for writing the compiler(like gcc) and unrelated with parsing expressions.

## Why the kmain.c build failed on my (virtual) system?

It is tested by the author on Fedora, some other platfrom maybe has no fpu.h or at other path.

## Why does not encourage the use of Windows in the note of Release but makes .exe and CI for it.

It is a closed-source(and non-libre) software(system). The .exe and CI is for testing the system independence(although some features lost), not for using on the target system. The macos in CI is like so.

## What is the predecessor of kmain.c

A simple module can only print messages to /dev/kmsg at init and exit, modeled after a helloworld module on the web.
kmalloc() and fpu_\*() was added for testing libexpr later, under the guidance of a GPT-like Chat.
