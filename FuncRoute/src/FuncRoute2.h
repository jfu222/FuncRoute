#ifndef __FUNC_ROUTE2_H__
#define __FUNC_ROUTE2_H__

#include <string>
#include <vector>


char cpp_keywords[][25] =
{
};

typedef enum _CPP_STEP_
{
	/*
	* C/C++语法元素
	*/
	CPP_STEP0_UNKNOWN = 0,    //未知语法元素

	/*
	* 步骤1：将原始代码，利用空白字符，分割成连续的 数字+字母+下划线 组成的单词
	*/
	CPP_STEP1_WORD_UNKNOWN,              //单词类型：未知，像运算符 +,-,*,/,,%,!,?,~,=,==,<,<=,=>,>,<<,>>,&,|,,||,^,//,/*,*/,",',#,;,:,::,[,],...这些 非 数字+字母+下划线 被归为此类
	CPP_STEP1_WORD_KEYWORD,              //单词类型：C/C++关键词 char,int,short,long,float,double,unsigned,if,else,for,do,while,private,protected,public,new,delete,class,struct,...
	CPP_STEP1_WORD_UNKNOWN_STR,          //单词类型：未知字符串

	/*
	* 步骤2：将 CPP_STEP1 已经归类的单词进一步两两配对，组合成更高一级的词法单元
	*/
	CPP_STEP2_WORDS_UNKNOWN,                     //词法单元：未知词语
	CPP_STEP2_WORDS_COMMENT_SINGLE_LINE,         //词法单元：代码的单行注释关键词 "//"
	CPP_STEP2_WORDS_COMMENT_MULTI_LINE,          //词法单元：代码的多行注释关键词 "/*...*/"
	CPP_STEP2_WORDS_PREPROCESSOR_UNKNOWN,        //词法单元：未知预处理命令 像 "#__has_include" 这些C++17定义的预处理命令，被归为此类
	CPP_STEP2_WORDS_PREPROCESSOR_IF,             //词法单元：预处理命令 "#if"
	CPP_STEP2_WORDS_PREPROCESSOR_ELIF,           //词法单元：预处理命令 "#elif"
	CPP_STEP2_WORDS_PREPROCESSOR_ELSE,           //词法单元：预处理命令 "#else"
	CPP_STEP2_WORDS_PREPROCESSOR_ENDIF,          //词法单元：预处理命令 "#endif"
	CPP_STEP2_WORDS_PREPROCESSOR_IFDEF,          //词法单元：预处理命令 "#ifdef"
	CPP_STEP2_WORDS_PREPROCESSOR_IFNDEF,         //词法单元：预处理命令 "#ifndef"
	CPP_STEP2_WORDS_PREPROCESSOR_DEFINE,         //词法单元：预处理命令 "#define"
	CPP_STEP2_WORDS_PREPROCESSOR_UNDEF,          //词法单元：预处理命令 "#undef"
	CPP_STEP2_WORDS_PREPROCESSOR_INCLUDE,        //词法单元：预处理命令 "#include"
	CPP_STEP2_WORDS_PREPROCESSOR_LINE,           //词法单元：预处理命令 "#line"
	CPP_STEP2_WORDS_PREPROCESSOR_ERROR,          //词法单元：预处理命令 "#error"
	CPP_STEP2_WORDS_PREPROCESSOR_PRAGMA,         //词法单元：预处理命令 "#pragma"
	CPP_STEP2_WORDS_PREPROCESSOR_DEFINED,        //词法单元：预处理命令 "#defined"
	CPP_STEP2_KEYWORD_ALIGNAS,                   //词法单元： "alignas", //(C++11 起)
	CPP_STEP2_KEYWORD_ALIGGNOF,                  //词法单元： "alignof", //(C++11 起)
	CPP_STEP2_KEYWORD_AND,                       //词法单元： "and",
	CPP_STEP2_KEYWORD_AND_EQ,                    //词法单元： "and_eq",
	CPP_STEP2_KEYWORD_ASM,                       //词法单元： "asm",
	CPP_STEP2_KEYWORD_ATOMIC_CANCEL,             //词法单元： "atomic_cancel", //(TM TS)
	CPP_STEP2_KEYWORD_ATOMIC_COMMIT,             //词法单元： "atomic_commit", //(TM TS)
	CPP_STEP2_KEYWORD_ATOMIC_NOEXCEPT,           //词法单元： "atomic_noexcept", //(TM TS)
	CPP_STEP2_KEYWORD_AUTO,                      //词法单元： "auto",
	CPP_STEP2_KEYWORD_BITAND,                    //词法单元： "bitand",
	CPP_STEP2_KEYWORD_BITOR,                     //词法单元： "bitor",
	CPP_STEP2_KEYWORD_BOOL,                      //词法单元： "bool",
	CPP_STEP2_KEYWORD_BREAK,                     //词法单元： "break",
	CPP_STEP2_KEYWORD_CASE,                      //词法单元： "case",
	CPP_STEP2_KEYWORD_CATCH,                     //词法单元： "catch",
	CPP_STEP2_KEYWORD_CHAR,                      //词法单元： "char",
	CPP_STEP2_KEYWORD_CHAR8_T,                   //词法单元： "char8_t", //(C++20 起)
	CPP_STEP2_KEYWORD_CHAR16_T,                  //词法单元： "char16_t", //(C++11 起)
	CPP_STEP2_KEYWORD_CHAR32_T,                  //词法单元： "char32_t", //(C++11 起)
	CPP_STEP2_KEYWORD_CLASS,                     //词法单元： "class",
	CPP_STEP2_KEYWORD_COMPL,                     //词法单元： "compl",
	CPP_STEP2_KEYWORD_CONCEPT,                   //词法单元： "concept", //(C++20 起)
	CPP_STEP2_KEYWORD_CONST,                     //词法单元： "const",
	CPP_STEP2_KEYWORD_CONSTEVAL,                 //词法单元： "consteval", //(C++20 起)
	CPP_STEP2_KEYWORD_CONSTEXPR,                 //词法单元： "constexpr", //(C++11 起)
	CPP_STEP2_KEYWORD_CONSTINIT,                 //词法单元： "constinit", //(C++20 起)
	CPP_STEP2_KEYWORD_CONST_CAST,                //词法单元： "const_cast",
	CPP_STEP2_KEYWORD_CONTINUE,                  //词法单元： "continue",
	CPP_STEP2_KEYWORD_CO_AWAIT,                  //词法单元： "co_await", //(C++20 起)
	CPP_STEP2_KEYWORD_CO_RETURN,                 //词法单元： "co_return", //(C++20 起)
	CPP_STEP2_KEYWORD_CO_YIELD,                  //词法单元： "co_yield", //(C++20 起)
	CPP_STEP2_KEYWORD_DECLTYPE,                  //词法单元： "decltype", //(C++11 起)
	CPP_STEP2_KEYWORD_DEFAULT,                   //词法单元： "default",
	CPP_STEP2_KEYWORD_DELETE,                    //词法单元： "delete",
	CPP_STEP2_KEYWORD_DO,                        //词法单元： "do",
	CPP_STEP2_KEYWORD_DOUBLE,                    //词法单元： "double",
	CPP_STEP2_KEYWORD_DYNAMIC_CAST,              //词法单元： "dynamic_cast",
	CPP_STEP2_KEYWORD_ELSE,                      //词法单元： "else",
	CPP_STEP2_KEYWORD_ENUM,                      //词法单元： "enum",
	CPP_STEP2_KEYWORD_EXPLICIT,                  //词法单元： "explicit",
	CPP_STEP2_KEYWORD_EXPORT,                    //词法单元： "export",
	CPP_STEP2_KEYWORD_EXTERN,                    //词法单元： "extern",
	CPP_STEP2_KEYWORD_FALSE,                     //词法单元： "false",
	CPP_STEP2_KEYWORD_FLOAT,                     //词法单元： "float",
	CPP_STEP2_KEYWORD_FOR,                       //词法单元： "for",
	CPP_STEP2_KEYWORD_FRIEND,                    //词法单元： "friend",
	CPP_STEP2_KEYWORD_GOTO,                      //词法单元： "goto",
	CPP_STEP2_KEYWORD_IF,                        //词法单元： "if",
	CPP_STEP2_KEYWORD_INLINE,                    //词法单元： "inline",
	CPP_STEP2_KEYWORD_INT,                       //词法单元： "int",
	CPP_STEP2_KEYWORD_LONG,                      //词法单元： "long",
	CPP_STEP2_KEYWORD_MUTABLE,                   //词法单元： "mutable",
	CPP_STEP2_KEYWORD_NAMESPACE,                 //词法单元： "namespace",
	CPP_STEP2_KEYWORD_NEW,                       //词法单元： "new",
	CPP_STEP2_KEYWORD_NOEXCEPT,                  //词法单元： "noexcept", //(C++11 起)
	CPP_STEP2_KEYWORD_NOT,                       //词法单元： "not",
	CPP_STEP2_KEYWORD_NOT_EQ,                    //词法单元： "not_eq",
	CPP_STEP2_KEYWORD_NULLPTR,                   //词法单元： "nullptr", //(C++11 起)
	CPP_STEP2_KEYWORD_OPERATOR,                  //词法单元： "operator",
	CPP_STEP2_KEYWORD_OR,                        //词法单元： "or",
	CPP_STEP2_KEYWORD_OR_EQ,                     //词法单元： "or_eq",
	CPP_STEP2_KEYWORD_PRIVATE,                   //词法单元： "private",
	CPP_STEP2_KEYWORD_PROTECTED,                 //词法单元： "protected",
	CPP_STEP2_KEYWORD_PUBLIC,                    //词法单元： "public",
	CPP_STEP2_KEYWORD_REFLEXPR,                  //词法单元： "reflexpr", //(反射 TS)
	CPP_STEP2_KEYWORD_REGISTER,                  //词法单元： "register",
	CPP_STEP2_KEYWORD_REINTERPRET_CAST,          //词法单元： "reinterpret_cast",
	CPP_STEP2_KEYWORD_REQUIRES,                  //词法单元： "requires", //(C++20 起)
	CPP_STEP2_KEYWORD_RETURN,                    //词法单元： "return",
	CPP_STEP2_KEYWORD_SHORT,                     //词法单元： "short",
	CPP_STEP2_KEYWORD_SIGNED,                    //词法单元： "signed",
	CPP_STEP2_KEYWORD_SIZEOF,                    //词法单元： "sizeof",
	CPP_STEP2_KEYWORD_STATIC,                    //词法单元： "static",
	CPP_STEP2_KEYWORD_STATIC_ASSERT,             //词法单元： "static_assert", //(C++11 起)
	CPP_STEP2_KEYWORD_STATIC_CAST,               //词法单元： "static_cast",
	CPP_STEP2_KEYWORD_STRUCT,                    //词法单元： "struct",
	CPP_STEP2_KEYWORD_SWITCH,                    //词法单元： "switch",
	CPP_STEP2_KEYWORD_SYNCHRONIZED,              //词法单元： "synchronized", //(TM TS)
	CPP_STEP2_KEYWORD_TEMPLATE,                  //词法单元： "template",
	CPP_STEP2_KEYWORD_THIS,                      //词法单元： "this",
	CPP_STEP2_KEYWORD_THREAD_LOCAL,              //词法单元： "thread_local", //(C++11 起)
	CPP_STEP2_KEYWORD_THROW,                     //词法单元： "throw",
	CPP_STEP2_KEYWORD_TRUE,                      //词法单元： "true",
	CPP_STEP2_KEYWORD_TRY,                       //词法单元： "try",
	CPP_STEP2_KEYWORD_TYPEDEF,                   //词法单元： "typedef",
	CPP_STEP2_KEYWORD_TYPEID,                    //词法单元： "typeid",
	CPP_STEP2_KEYWORD_TYPENAME,                  //词法单元： "typename",
	CPP_STEP2_KEYWORD_UNION,                     //词法单元： "union",
	CPP_STEP2_KEYWORD_UNSIGNED,                  //词法单元： "unsigned",
	CPP_STEP2_KEYWORD_USING,                     //词法单元： "using",
	CPP_STEP2_KEYWORD_VIRTUAL,                   //词法单元： "virtual",
	CPP_STEP2_KEYWORD_VOID,                      //词法单元： "void",
	CPP_STEP2_KEYWORD_VOLATILE,                  //词法单元： "volatile",
	CPP_STEP2_KEYWORD_WCHAR_T,                   //词法单元： "wchar_t",
	CPP_STEP2_KEYWORD_WHILE,                     //词法单元： "while",
	CPP_STEP2_KEYWORD_XOR,                       //词法单元： "xor",
	CPP_STEP2_KEYWORD_XOR_EQ,                    //词法单元： "xor_eq",
	CPP_STEP2_WORDS_OPERATOR_PAIR,               //词法单元：将配对的运算符组成一组，类似 "()","{}","[]","<>",...

	/*
	* 在 CPP_STEP2 的基础上，将相近的词法单元连接起来，组成一条单独的语句，
	* 并将每个词法单元进行分类
	*/
	CPP_STEP3_STATEMENT_UNKNOWN,                       //句法单元：未知
	CPP_STEP3_STATEMENT_END_SYMBOL,                    //句法单元：语句结束符，类似 "int a;" 中的";"
	CPP_STEP3_STATEMENT_DECLARE_TYPE,                  //句法单元：变量的声明类型，例如 "static unsigned long long"
	CPP_STEP3_STATEMENT_CLASS_NAME,                    //句法单元：类名
	CPP_STEP3_STATEMENT_STRUCT_NAME,                   //句法单元：结构体名
	CPP_STEP3_STATEMENT_VAR_NAME,                      //句法单元：变量名
	CPP_STEP3_STATEMENT_FUNCTION_NAME,                 //句法单元：函数名
	CPP_STEP3_STATEMENT_ENUM_NAME,                     //句法单元：枚举名
	CPP_STEP3_STATEMENT_UNION_NAME,                    //句法单元：共用体名
	CPP_STEP3_STATEMENT_ARRAY_NAME,                    //句法单元：数组名
	CPP_STEP3_STATEMENT_VAR_DECLARE,                   //句法单元：变量声明，类似 "int a;" 中的"a"
	CPP_STEP3_STATEMENT_CLASS_DECLARE,                 //句法单元：C++类声明，类似 "class A : public B {};"
	CPP_STEP3_STATEMENT_STRUCT_DECLARE,                //句法单元：C++结构体声明，类似 "typedef struct _A_ {}A;"
	CPP_STEP3_STATEMENT_FUNCTION_DECLARE_RETURN,       //句法单元：函数声明的返回值，类似 "virtual int get() = 0;" 中的"virtual int"
	CPP_STEP3_STATEMENT_FUNCTION_DECLARE_NAME,         //句法单元：函数声明的函数名，类似 "virtual int get() = 0;" 中的"get"
	CPP_STEP3_STATEMENT_FUNCTION_DECLARE_ARGS,         //句法单元：函数声明的参数列表，类似 "virtual int get() = 0;" 中的"()"
	CPP_STEP3_STATEMENT_FUNCTION_DECLARE_QUALIFIER,    //句法单元：函数声明的限定修饰符，类似 "virtual int get() = 0;" 中的"= 0"
	CPP_STEP3_STATEMENT_FUNCTION_DEFINE_RETURN,        //句法单元：函数定义的返回值，类似 "int A::get(){ return 0；}" 中的"int"
	CPP_STEP3_STATEMENT_FUNCTION_DEFINE_NAME,          //句法单元：函数定义的函数名，类似 "int A::get(){ return 0；}" 中的"A::get"
	CPP_STEP3_STATEMENT_FUNCTION_DEFINE_ARGS,          //句法单元：函数定义的参数列表，类似 "int A::get(){ return 0；}" 中的"()"
	CPP_STEP3_STATEMENT_FUNCTION_DEFINE_BODY,          //句法单元：函数定义的体，类似 "int A::get(){ return 0；}" 中的"{ return 0；}"
	CPP_STEP3_STATEMENT_IF_DEFINE,                     //句法单元：if条件判断定义，类似 "if (a == 1){ return 0；}"
	CPP_STEP3_STATEMENT_ELSE_IF_DEFINE,                //句法单元：else if条件判断定义，类似 "else if (a == 1){ return 0；}"
	CPP_STEP3_STATEMENT_ELSE_DEFINE,                   //句法单元：else条件判断定义，类似 "else { return 0；}"
	CPP_STEP3_STATEMENT_FOR_DEFINE,                    //句法单元：for循环定义，类似 "for (int i = 0; i < 3; ++i){ a += i；}"
	CPP_STEP3_STATEMENT_WHILE_DEFINE,                  //句法单元：while条件循环定义，类似 "while (a < 3){ a += 1；}"
	CPP_STEP3_STATEMENT_DO_WHILE_DEFINE,               //句法单元：do while条件循环定义，类似 "do { a += 1；} while (a < 3);"
	CPP_STEP3_STATEMENT_SWITCH_DEFINE,                 //句法单元：switch定义，类似 "switch (a) { case 1: {break;} case 2: {break;} default: {break;} }"
	CPP_STEP3_STATEMENT_TRY_CATCH,                     //句法单元：try catch异常捕获，类似 "try { a += 1；} catch (int err){}"
	CPP_STEP3_STATEMENT_MACRO_NAME,                    //句法单元：宏定义的名称，类似 "#define PI 3.1415926" 中的"PI"
	CPP_STEP3_STATEMENT_MACRO_ARGS,                    //句法单元：宏定义的参数列表，类似 "SUB(a, b)  (a - b)" 中的"(a, b)"
	CPP_STEP3_STATEMENT_MACRO_BODY,                    //句法单元：宏定义的体，类似 "#define PI 3.1415926" 中的"3.1415926"

	/*
	* 在 CPP_STEP3 的基础上，将一条单独的语句，解释成抽象的语义
	*/
	CPP_STEP4_SEMANTICS_UNKNOWN,                      //语义单元：未知
	CPP_STEP4_SEMANTICS_NORMAL_VAR,                   //语义单元：普通变量，类似 "int a;"
	CPP_STEP4_SEMANTICS_POINTER_VAR,                  //语义单元：指针变量，类似 "int *a = NULL;"
	CPP_STEP4_SEMANTICS_STATIC_VAR,                   //语义单元：静态变量，类似 "static int a = 0;"
	CPP_STEP4_SEMANTICS_GLOBAL_VAR,                   //语义单元：全局变量，类似 "int g_a = 0;"
	CPP_STEP4_SEMANTICS_CLASS_MEMBER_VAR,             //语义单元：C++类成员变量，类似 "class A {int m_a;};"
	CPP_STEP4_SEMANTICS_CLASS_MEMBER_FUNCTION,        //语义单元：C++类成员函数，类似 "class A {int get();};"
	CPP_STEP4_SEMANTICS_FUNCTION_BODY_VAR,            //语义单元：函数体内部声明的变量，类似 "int get() {int a;}" 中的"a"
	CPP_STEP4_SEMANTICS_FUNCTION_BODY_FUNC_CALLED,    //语义单元：函数体内部的函数调用，类似 "int get() {printf("");}" 中的"printf"
	CPP_STEP4_SEMANTICS_FUNCTION_ARGS_VAR,            //语义单元：函数参数列表声明的变量，类似 "int get(int a) {}" 中的"a"
	CPP_STEP4_SEMANTICS_CONDITION,                    //语义单元：条件判断分支，类似 "if(){}else{}"
	CPP_STEP4_SEMANTICS_LOOP,                         //语义单元：循环，类似 "for(){}"
}CPP_STEP;


typedef struct _MY_STRING2_
{
	char str[256]; //字符串
}MY_STRING2;


typedef struct _STRING_POSITON2_
{
	unsigned char *start;
	unsigned char *end;
	int fileOffsetOfStart;
	int fileOffsetOfEnd;
	int lineNumberOfStart;
	int lineNumberOfEnd;
	int length;
	char str[1024];

public:
	int copyStrFromBuffer()
	{
		int ret = 0;
		if (start == NULL){ return -1; }

		if (str == NULL){ printf("Error: malloc() failed!\n"); return -1; }
		int len = length;
		if (len > sizeof(str) - 1)
		{
			len = sizeof(str) - 1;
		}

		if (len > 0)
		{
			memcpy(str, start, len);
			str[len] = '\0';
		}
		return 0;
	}
}STRING_POSITON2;


typedef struct _CPP_STEP_INDEX_
{
	CPP_STEP step; //语法分析后的归类
	std::vector<int> indexes; //vStep1的下标数组
}CPP_STEP_INDEX;


typedef struct _CPP_STEPS_
{
	std::vector<STRING_POSITON2> vStep1;
	std::vector<CPP_STEP_INDEX> vStep2;
	std::vector<CPP_STEP_INDEX> vStep3;
	std::vector<CPP_STEP_INDEX> vStep4;
}CPP_STEPS;


//---------C/C++源代码文件函数调用关系类-----------------
class CFuncRoute2
{
public:
	std::string m_srcCodesFilePath; //C/C++源代码文件路径
	std::vector<std::string> m_fileSuffixes; //C/C++源代码文件后缀名（忽略大小写）数组，例如 [".h", ".hpp", ".c", ".cpp", ".cc", "*"]

public:
	CFuncRoute2();
	~CFuncRoute2();
};

#endif //__FUNC_ROUTE2_H__
