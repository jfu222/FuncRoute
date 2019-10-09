#ifndef __CPP_KEYWORD_H__
#define __CPP_KEYWORD_H__


//C++关键字
char cpp_keywords[][25] =
{
    "alignas", //(C++11 起)
    "alignof", //(C++11 起)
    "and",
    "and_eq",
    "asm",
    "atomic_cancel", //(TM TS)
    "atomic_commit", //(TM TS)
    "atomic_noexcept", //(TM TS)
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "char8_t", //(C++20 起)
    "char16_t", //(C++11 起)
    "char32_t", //(C++11 起)
    "class",
    "compl",
    "concept", //(C++20 起)
    "const",
    "consteval", //(C++20 起)
    "constexpr", //(C++11 起)
    "constinit", //(C++20 起)
    "const_cast",
    "continue",
    "co_await", //(C++20 起)
    "co_return", //(C++20 起)
    "co_yield", //(C++20 起)
    "decltype", //(C++11 起)
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept", //(C++11 起)
    "not",
    "not_eq",
    "nullptr", //(C++11 起)
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "reflexpr", //(反射 TS)
    "register",
    "reinterpret_cast",
    "requires", //(C++20 起)
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert", //(C++11 起)
    "static_cast",
    "struct",
    "switch",
    "synchronized", //(TM TS)
    "template",
    "this",
    "thread_local", //(C++11 起)
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq",
    "__attribute__", //GNU C/C++，排除类似 const struct __attribute__((packed)) { uint32_t v; } *q = p; 这样的BT写法
};


//C++预处理命令
char cpp_preprocessors[][25] =
{
    "if",
    "elif",
    "else",
    "endif",
    "ifdef",
    "ifndef",
    "define",
    "undef",
    "include",
    "line",
    "error",
    "pragma",
    "defined",
    "__has_include", // (C++17 起)
    "__has_cpp_attribute", // (C++20 起)
};

#endif //__CPP_KEYWORD_H__
