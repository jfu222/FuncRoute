#ifndef __COMMON_DATA_H__
#define __COMMON_DATA_H__

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <stack>


typedef enum _VAR_TYPE_
{
	VAR_TYPE_UNKNOWN = 0, //未知变量
	VAR_TYPE_NORMAL, //正常变量
	VAR_TYPE_POINTER, //指针变量
	VAR_TYPE_STATIC1, //静态变量 static a = 0;
	VAR_TYPE_STATIC2, //静态变量 static A::m_a = 0;
}VAR_TYPE;


typedef struct _MY_STRING_
{
	char str[256]; //字符串
}MY_STRING;


typedef struct _STRING_POSITON_
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
	int printfInfo()
	{
		printf("-----STRING_POSITON----START---\n");
		printf("start: %p;\n", start);
		printf("end: %p;\n", end);
		printf("fileOffsetOfStart: %d;\n", fileOffsetOfStart);
		printf("fileOffsetOfEnd: %d;\n", fileOffsetOfEnd);
		printf("lineNumberOfStart: %d;\n", lineNumberOfStart);
		printf("lineNumberOfEnd: %d;\n", lineNumberOfEnd);
		printf("length: %d;\n", length);
		printf("str: ---%s---\n", str);
		printf("-----STRING_POSITON----END---\n");

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
	VAR_TYPE instanceType; //类的实例化对象名类型
	int functionIndex; //函数编号，全局唯一

public:
	int printfInfo()
	{
		printf("-----CLASS_INSTANCE----START--%d---\n", functionIndex);
		printf("functionIndex: %d;\n", functionIndex);
		printf("instanceType: %d;\n", instanceType);

		className.printfInfo();
		classInstanceName.printfInfo();
		functionName.printfInfo();
		functionArgs.printfInfo();
		functionReturnValue.printfInfo();
		printf("-----CLASS_INSTANCE----END--%d---\n", functionIndex);

		return 0;
	}
}CLASS_INSTANCE;


//C++变量声明
typedef struct _VAR_DECLARE_
{
	STRING_POSITON varType; //变量类型
	STRING_POSITON varName; //变量名
	bool isPointerVar; //是否是指针变量变量名
	int varIndex; //变量编号

public:
	int printfInfo()
	{
		printf("-----VAR_DECLARE----START--%d---\n", varIndex);
		printf("varIndex: %d;\n", varIndex);
		printf("isPointerVar: %d;\n", isPointerVar);
		varType.printfInfo();
		varName.printfInfo();
		printf("-----VAR_DECLARE----END--%d---\n", varIndex);

		return 0;
	}
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

public:
	int printfInfo()
	{
		printf("-----FUNCTION_STRUCTURE---START--%d---\n", functionIndex);
		functionReturnValueTypeQualifier.printfInfo();
		functionReturnValue.printfInfo();
		functionName.printfInfo();
		functionParameter.printfInfo();
		functionTypeQualifier.printfInfo();
		functionBody.printfInfo();

		int len = funcsWhichInFunctionBody.size();
		for (int i = 0; i < len; ++i)
		{
			printf("funcsWhichInFunctionBody[%d]:\n", i);
			funcsWhichInFunctionBody[i].printfInfo();
		}
		printf("className: %s;\n", className);
		printf("structName: %s;\n", structName);
		printf("classNameAlias: %s;\n", classNameAlias);
		printf("funcString: %s;\n", funcString);
		printf("functionIndex: %d;\n", functionIndex);
		printf("-----FUNCTION_STRUCTURE---END--%d---\n", functionIndex);

		return 0;
	}
}FUNCTION_STRUCTURE;


typedef struct _CLASS_STRUCT_
{
	STRING_POSITON className; //类/结构体名
	STRING_POSITON classNameAlias; //类/结构体的别名
	STRING_POSITON classBody; //类的体
	STRING_POSITON classParent; //父类
	std::vector<MY_STRING> classParents; //父类们（可以包含父类的父类），例如："class B : public A, public C{};"
	std::vector<VAR_DECLARE> memberVars; //成员变量
	std::vector<FUNCTION_STRUCTURE> memberFuncs; //成员函数
	bool isStruct; //是否是结构体

public:
	int printfInfo()
	{
		printf("-----CLASS_STRUCT---START--isStruct=%d---\n", isStruct);
		printf("isStruct: %d;\n", isStruct);
		className.printfInfo();
		classNameAlias.printfInfo();
		classBody.printfInfo();
		classParent.printfInfo();

		int len = classParents.size();
		for (int i = 0; i < len; ++i)
		{
			printf("classParents[%d]: %s;\n", i, classParents[i].str);
		}

		len = memberVars.size();
		for (int i = 0; i < len; ++i)
		{
			printf("memberVars[%d]:\n", i);
			memberVars[i].printfInfo();
		}

		len = memberFuncs.size();
		for (int i = 0; i < len; ++i)
		{
			printf("memberFuncs[%d]:\n", i);
			memberFuncs[i].printfInfo();
		}

		printf("-----CLASS_STRUCT---END--isStruct=%d---\n", isStruct);

		return 0;
	}
}CLASS_STRUCT;


typedef struct _FUNCTIONS_
{
	char fllename[600]; //所在文件名
	std::vector<FUNCTION_STRUCTURE> funcs;
	std::vector<CLASS_STRUCT> classes; //存储本文件中声明了哪些C++类/结构体

public:
	int printfInfo()
	{
		printf("-----FUNCTIONS---START---fllename=%s--\n", fllename);
		printf("fllename: %s;\n", fllename);

		int len = funcs.size();
		for (int i = 0; i < len; ++i)
		{
			printf("funcs[%d]:\n", i);
			funcs[i].printfInfo();
		}

		len = classes.size();
		for (int i = 0; i < len; ++i)
		{
			printf("classes[%d]:\n", i);
			classes[i].printfInfo();
		}

		printf("-----FUNCTIONS---END---fllename=%s--\n", fllename);

		return 0;
	}
}FUNCTIONS;


typedef struct _FUNC_INDEX_
{
	int index1;
	int index2;
	int funcIndex;
	int treeDepth;
	int refCount;
	int lastI; //主要用于生成 pdf 时需要
	int lastJ; //主要用于生成 pdf 时需要
	int parentIndexTemp; //主要用于生成 pdf 时需要
	std::vector<_FUNC_INDEX_ *> parentIndexs; //同一个函数可能被好几个函数调用
	std::vector<_FUNC_INDEX_ *> childrenIndexs; //该函数调用了哪些其他函数（可以自己调用自己）

public:
	_FUNC_INDEX_();
	~_FUNC_INDEX_();

	bool isRecursiveFunction(int funcIndex, std::string &strChain); //是否是递归函数，包含：A->A，以及 A->B->A
	bool isRecursiveFunctionExplicitCalled(int funcIndex); //是否是显式递归函数，即A->A，不是 A->B->A
	int freeMem();
	int printInfo();
	int printInfoFuncRoute(std::vector<_FUNC_INDEX_ *> &funcs);
}FUNC_INDEX;


typedef struct _FUNC_INDEX_POS_
{
	FUNC_INDEX * node;
	int colBase;
	int rowBase;
}FUNC_INDEX_POS;


typedef struct _FUNCS_CALLED_TREE_
{
	std::vector<FUNC_INDEX *> funcsIndexs;

public:
	int freeMem()
	{
		int len1 = funcsIndexs.size();
		for (int i = 0; i < len1; ++i)
		{
			int len2 = funcsIndexs[i]->childrenIndexs.size();
			for (int j = 0; j < len2; ++j)
			{
				funcsIndexs[i]->childrenIndexs[j]->freeMem();
			}
		}
		return 0;
	}
}FUNCS_CALLED_TREE;


typedef struct _MACRO_
{
	char macroName[256]; //宏名
	char macroArgs[256]; //宏名参数列表
	char macroBody[1024]; //宏体

public:
	int printfInfo()
	{
		printf("-----MACRO---START----\n");
		printf("macroName: %s;\n", macroName);
		printf("macroArgs: %s;\n", macroArgs);
		printf("macroBody: %s;\n", macroBody);
		printf("-----MACRO---END----\n");

		return 0;
	}
}MACRO;

#endif //__COMMON_DATA_H__
