#ifndef __FUNC_ROUTE_H__
#define __FUNC_ROUTE_H__

#include <string>
#include <vector>
#include <map>


typedef struct _MY_STRING_
{
	char str[256]; //字符串
}MY_STRING;


typedef struct _STRING_POSITON_
{
	unsigned char *start;
	unsigned char *end;
	unsigned int fileOffsetOfStart;
	unsigned int fileOffsetOfEnd;
	unsigned int lineNumberOfStart;
	unsigned int lineNumberOfEnd;
	unsigned int length;
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
}STRING_POSITON;


//C++类/结构体的实例化对象
typedef struct _CLASS_INSTANCE_
{
	STRING_POSITON className; //类名
	STRING_POSITON classInstanceName; //类的实例化对象名
	STRING_POSITON functionName; //调用的是类的哪一个函数
	STRING_POSITON functionArgs; //函数的参数
	STRING_POSITON functionReturnValue; //函数返回值
	int functionIndex; //函数编号，全局唯一
}CLASS_INSTANCE;


//C++变量声明
typedef struct _VAR_DECLARE_
{
	STRING_POSITON varType; //变量类型
	STRING_POSITON varName; //变量名
	int varIndex; //变量编号
}VAR_DECLARE;


typedef struct _FUNCTION_STRUCTURE_
{
	STRING_POSITON functionReturnValueTypeQualifier; //函数返回值(type-qualifier类型限定符)： const，template, virtual, inline, static, extern, explicit, friend, constexpr
	STRING_POSITON functionReturnValue; //函数返回值类型(type-specifier类型区分符): void *, void, int, short, long, float, double , auto, struct结构体类型，enum枚举类型，typedef类型
	STRING_POSITON functionName; //函数名
	STRING_POSITON functionParameter; //函数参数
	STRING_POSITON functionTypeQualifier; //函数参数右小括号后面紧跟的修饰符(type-qualifier类型限定符)：=0, =default, =delete, const，voliate, &(左值引用限定符), &&(右值引用限定符), override, final, noexcept, throw
	STRING_POSITON functionBody; //函数体
	std::vector<CLASS_INSTANCE> funcsWhichInFunctionBody; //函数体内部，调用了哪些其他函数
//	std::map<int, int> funcsWhichCalledMe; //该函数被哪些函数(使用函数编号表示)调用了（可能会被同一个函数多次调用）
	char className[200]; //函数所在的C++类名称
	char structName[200]; //函数所在的C++结构体名称
	char classNameAlias[200]; //类/结构体的别名
	char funcString[1024]; //函数返回值 + 函数名 + 函数参数
	int functionIndex; //函数编号，全局唯一
}FUNCTION_STRUCTURE;


typedef struct _CLASS_STRUCT_
{
	STRING_POSITON className; //类/结构体名
	STRING_POSITON classNameAlias; //类/结构体的别名
	STRING_POSITON classBody; //类的体
	STRING_POSITON classParent; //父类
	std::vector<MY_STRING> classParents; //父类们（可以包含父类的父类），例如："class B : public A, public C{};"
	std::vector<VAR_DECLARE> memberVars; //成员变量
	std::vector<FUNCTION_STRUCTURE> memberFunc; //成员函数
	bool isStruct; //是否是结构体
}CLASS_STRUCT;


typedef struct _FUNCTIONS_
{
	unsigned char fllename[600]; //所在文件名
	std::vector<FUNCTION_STRUCTURE> funcs;
	std::vector<CLASS_STRUCT> classes; //存储本文件中声明了哪些C++类/结构体
}FUNCTIONS;


typedef struct _MACRO_
{
	char macroName[256]; //宏名
	char macroArgs[256]; //宏名参数列表
	char macroBody[1024]; //宏体
}MACRO;


//---------C/C++源代码文件函数调用关系类-----------------
class CFuncRoute
{
public:
	std::string m_srcCodesFilePath; //C/C++源代码文件路径
	std::vector<std::string> m_fileSuffixes; //C/C++源代码文件后缀名（忽略大小写）数组，例如 [".h", ".hpp", ".c", ".cpp", ".cc", "*"]

public:
	CFuncRoute();
	~CFuncRoute();

	int findAllFunctionsName(std::string filePath, std::vector<std::string> suffixes); //从源代码文件里面，提取出所有函数名
	int search_C_FuncName(unsigned char *buffer, unsigned int bufferSize, FUNCTIONS &functions); //从内存buffer中，搜索C语言函数名
	int search_CPP_FuncName(unsigned char *buffer, unsigned int bufferSize, FUNCTIONS &functions); //从内存buffer中，搜索C++语言函数名
	bool isKeyword(unsigned char *buffer, int bufferSize); //字符串是否是C/C++语言关键词
	int replaceAllCodeCommentsBySpace(unsigned char *buffer, int bufferSize); //将所有用"//..."或"/*...*/"注释掉的代码用空格' '代替
	int findStr(unsigned char *buffer, int bufferSize, const char *str, int &pos); //在内存中，查找指定的字符串
	int findAllMacros(std::vector<std::string> files, std::vector<MACRO> &macros); //从所有代码源文件中，找到所有的宏定义
	int findAllFuncsInFunctionBody(unsigned char *buffer, int bufferSize, std::vector<CLASS_INSTANCE> &funcsWhichInFunctionBody, unsigned char *bufferBase, int lineNumberBase); //查找函数体内部调用的所有其他函数
	int macroExpand(); //将宏定义展开
	bool isParentClass(std::string child, std::string parent, std::vector<FUNCTIONS> &vFunctions); //判断parent类是否是child的父类
	int splitParentsClass(unsigned char *buffer, int bufferSize, std::vector<MY_STRING> &classParents); //拆分父类们，例如："class B : public A, public C{};"
	int updateParentClass(std::vector<FUNCTIONS> &vFunctions); //主要解决两层以上的父类继承
	int findAllMemberVarsInClassDeclare(unsigned char *buffer, int bufferSize, CLASS_STRUCT &classes, unsigned char *bufferBase, int lineNumberBase); //在类/结构体的声明语句块内部，提取出所有声明的成员变量
	bool isFunctionArgsMatch(std::string parameter, std::string functionArgs); //函数的声明的参数列表和函数的调用传入的参数列表是否匹配
	int statAllFuns(std::vector<FUNCTIONS> &vFunctions); //统计所有函数之间的调用关系
};

#endif //__FUNC_ROUTE_H__
