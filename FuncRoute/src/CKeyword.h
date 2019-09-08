#ifndef __C_KEYWORD_H__
#define __C_KEYWORD_H__


//C关键字
char c_keywords[][25] =
{
    "auto",
    "break",
    "case",
    "char",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extern",
    "float",
    "for",
    "goto",
    "if",
    "inline", //(C99 起)
    "int",
    "long",
    "register",
    "restrict", //(C99 起)
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "struct",
    "switch",
    "typedef",
    "union",
    "unsigned",
    "void",
    "volatile",
    "while",
    "_Alignas", //(C11 起)
    "_Alignof", //(C11 起)
    "_Atomic", //(C11 起)
    "_Bool", //(C99 起)
    "_Complex", //(C99 起)
    "_Generic", //(C11 起)
    "_Imaginary", //(C99 起)
    "_Noreturn", //(C11 起)
    "_Static_assert", //(C11 起)
    "_Thread_local", //(C11 起)
};


//C预处理命令
char c_preprocessors[][25] =
{
    "if",
    "elif",
    "else",
    "endif",
    "defined",
    "ifdef",
    "ifndef",
    "define",
    "undef",
    "include",
    "line",
    "error",
    "pragma",
};

#endif //__C_KEYWORD_H__
