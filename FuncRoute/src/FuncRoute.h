#ifndef __FUNC_ROUTE_H__
#define __FUNC_ROUTE_H__

#include <string>
#include <vector>
#include <map>
#include <queue>


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
	unsigned char fllename[600]; //所在文件名
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
	std::vector<_FUNC_INDEX_ *> parentIndexs; //同一个函数可能被好几个函数调用
	std::vector<_FUNC_INDEX_ *> childrenIndexs; //该函数调用了哪些其他函数（可以自己调用自己）

public:
	bool isRecursiveFunction(int funcIndex) //是否是递归函数
	{
		int len = parentIndexs.size();
		if(len == 0)
		{
			return false;
		}else
		{
			for(int i = 0; i < len; ++i)
			{
				if(parentIndexs[i]->funcIndex == funcIndex)
				{
					return true;
				}else
				{
					bool ret = parentIndexs[i]->isRecursiveFunction(funcIndex);
					if(ret == true)
					{
						return true;
					}
				}
			}
		}
		
		return false;
	}
	
	int freeMem()
	{
		int len1 = childrenIndexs.size();
		for(int i = 0; i < len1; ++i)
		{
			if(childrenIndexs[i])
			{
				childrenIndexs[i]->freeMem();
				free(childrenIndexs[i]);
				childrenIndexs[i] = NULL;
			}
		}
		return 0;
	}
	
	int printInfo()
	{
		int ret = 0;

		std::vector<_FUNC_INDEX_ *> stack;
		std::vector<int> stackParent;
		std::vector<int> stackDepth;

		//---------------------------------
		printf("==========\n");
		stack.push_back(this);
		stackParent.push_back(this->funcIndex);
		stackDepth.push_back(1);
		
		while (stack.empty() == false)
		{
			_FUNC_INDEX_ *node = stack.front();
			int parentIndex = stackParent.front();
			int depth = stackDepth.front();

			stack.erase(stack.begin());
			stackParent.erase(stackParent.begin());
			stackDepth.erase(stackDepth.begin());

			if(stack.empty() == true)
			{
				printf("%d\n", node->funcIndex); //换行
			}else
			{
				//---------------------
				if (parentIndex == stackParent[0]) //共同的父节点
				{
					printf("%d-", node->funcIndex);
				} else //非共同的父节点
				{
					if (depth == stackDepth[0]) //深度相等
					{
						printf("%d ", node->funcIndex);
					} else
					{
						printf("%d\n", node->funcIndex);
					}
				}
			}
			
			bool bRet = node->isRecursiveFunction(node->funcIndex);
			if(bRet == false) //不是递归函数
			{
				int len = node->childrenIndexs.size();
				for(int i = 0; i < len; ++i)
				{
					stack.push_back(node->childrenIndexs[i]);
					stackParent.push_back(node->funcIndex);
					stackDepth.push_back(depth + 1);
				}
			}
		}
		printf("\n");
		return 0;
	}

	int printInfoFuncRoute(std::vector<_FUNC_INDEX_ *> &funcs)
	{
		int ret = 0;
		
		int len1 = this->childrenIndexs.size();
		int len2 = funcs.size();
		bool bRet = this->isRecursiveFunction(this->funcIndex);

		if(len1 == 0 || bRet == true)
		{
			if(len2 != 0)
			{
				printf("[");
				for (int i = 0; i < len2; ++i)
				{
					if(i == len2 - 1)
					{
						printf("%d", funcs[i]->funcIndex);
					}else
					{
						printf("%d-", funcs[i]->funcIndex);
					}
				}
				printf("]\n");
			}else
			{
				printf("[%d]\n", this->funcIndex);
			}
		}else
		{
			if(bRet == false)
			{
				for (int i = 0; i < len1; ++i)
				{
					funcs.push_back(this->childrenIndexs[i]);
					this->childrenIndexs[i]->printInfoFuncRoute(funcs);
					funcs.pop_back();
				}
			}
		}

		return 0;
	}

}FUNC_INDEX;


typedef struct _FUNCS_CALLED_TREE_
{
	std::vector<FUNC_INDEX *> funcsIndexs;

public:
	int freeMem()
	{
		int len1 = funcsIndexs.size();
		for(int i = 0; i < len1; ++i)
		{
			int len2 = funcsIndexs[i]->childrenIndexs.size();
			for(int j = 0; j < len2; ++j)
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


//---------C/C++源代码文件函数调用关系类-----------------
class CFuncRoute
{
public:
	std::string m_srcCodesFilePath; //C/C++源代码文件路径
	std::vector<std::string> m_fileSuffixes; //C/C++源代码文件后缀名（忽略大小写）数组，例如 [".h", ".hpp", ".c", ".cpp", ".cc", "*"]

public:
	CFuncRoute();
	~CFuncRoute();

	int CFuncRoute::splitDirsBySemicolon(std::string dirs, std::vector<std::string> &vecDirs); //用分号';'分开字符串
	int findAllFunctionsName(std::vector<std::string> dirsInclude, std::vector<std::string> fileDirsExclude, std::vector<std::string> suffixes); //从源代码文件里面，提取出所有函数名
	int search_C_FuncName(unsigned char *buffer, unsigned int bufferSize, FUNCTIONS &functions); //从内存buffer中，搜索C语言函数名
	int search_CPP_FuncName(unsigned char *buffer, unsigned int bufferSize, FUNCTIONS &functions); //从内存buffer中，搜索C++语言函数名
	bool isKeyword(unsigned char *buffer, int bufferSize); //字符串是否是C/C++语言关键词
	bool isKeywordVarType(unsigned char *buffer, int bufferSize); //字符串是否是C/C++语言变量关键词
	int findOverloadOperatorsBack(unsigned char *buffer, int bufferSize, unsigned char *rightCharPos, unsigned char *&leftCharPos); //查找是否是C++可重载运算符
	int replaceAllCodeCommentsBySpace(unsigned char *buffer, int bufferSize); //将所有用"//..."或"/*...*/"注释掉的代码用空格' '代替
	int replaceAllStrBySpace(unsigned char *buffer, int bufferSize); //将所有用双引号""的代码用空格' '代替
	int replaceAllMacroDefineStrBySpace(unsigned char *buffer, int bufferSize); //将所有#define宏定义用空格' '代替
	int findStr(unsigned char *buffer, int bufferSize, const char *str, int &pos); //在内存中，查找指定的字符串
	int findAllMacros(std::vector<std::string> files, std::vector<MACRO> &macros); //从所有代码源文件中，找到所有的宏定义
	int findAllClassAndStructDeclare(unsigned char *buffer, int bufferSize, std::vector<CLASS_STRUCT> &classes); //在内存中查找所有完整的C++类/结构体声明
	int findAllFuncsInFunctionBody(unsigned char *buffer, int bufferSize, std::vector<CLASS_INSTANCE> &funcsWhichInFunctionBody, unsigned char *bufferBase, int lineNumberBase); //查找函数体内部调用的所有其他函数
	int findWholeFuncCalled(unsigned char *buffer, int bufferSize, unsigned char *parentheseLeft, CLASS_INSTANCE &classInstance, unsigned char *bufferBase, int lineNumberBase); //给定左小括号'('的位置，返回一个完整的函数调用
	int findWholeFuncDeclare(unsigned char *buffer, int bufferSize, unsigned char *parentheseLeft, FUNCTION_STRUCTURE &funcDeclare, unsigned char *bufferBase, int lineNumberBase); //给定左小括号'('的位置，返回一个完整的函数声明
	int findNextMacroDefine(unsigned char *buffer, int bufferSize, unsigned char *&leftPos, unsigned char *&rightCharPos); //查找并返回下一个完整的#define宏声明
	int findNextCodeComments(unsigned char *buffer, int bufferSize, unsigned char *&leftPos, unsigned char *&rightCharPos); //查找并返回下一个完整的注释语句

	int skipWhiteSpaceForward(unsigned char *buffer, int bufferSize, unsigned char *leftPos, unsigned char *&rightPos, int &lineNumber); //前向跳过空白字符
	int skipWhiteSpaceBack(unsigned char *buffer, int bufferSize, unsigned char *rightPos, unsigned char *&leftPos, int &lineNumber); //反向跳过空白字符
	int findPairCharForward(unsigned char *buffer, int bufferSize, unsigned char *leftCharPos, char leftChar, char rightChar, unsigned char *&rightCharPos); //前向查找配对字符，比如"{}","<>","()","[]","''"
	int findPairCharBack(unsigned char *buffer, int bufferSize, unsigned char *rightCharPos, char leftChar, char rightChar, unsigned char *&leftCharPos); //反向查找配对字符，比如"{}","<>","()","[]","''"
	int findPairCharBackStop(unsigned char *buffer, int bufferSize, unsigned char *rightCharPos, char leftChar, char rightChar, char *stopChar, unsigned char *&leftCharPos); //反向查找配对字符，遇到停止符则返回失败，比如"{}","<>","()","[]","''"
	int findCharForward(unsigned char *buffer, int bufferSize, char ch, unsigned char *&rightCharPos); //前向查找指定字符
	int findCharForwardStop(unsigned char *buffer, int bufferSize, char ch, char *stopChar, unsigned char *&rightCharPos); //前向查找指定字符，遇到停止符则返回失败
	int findCharBack(unsigned char *buffer, int bufferSize, char ch, unsigned char *&leftCharPos); //反向查找指定字符
	int findCharsBackGreedy(unsigned char *buffer, int bufferSize, char *chars, unsigned char *&leftCharPos); //反向查找指定的某几个字符，贪婪查找直到找到为止
	int findCharBackStop(unsigned char *buffer, int bufferSize, char ch, char *stopChar, unsigned char *&leftCharPos); //反向查找指定字符，遇到停止符则返回失败
	int findCharStrsBackStop(unsigned char *buffer, int bufferSize, char ch, char *stopChar, const char stopStrs[][20], int stopStrsSize, unsigned char *&leftCharPos); //反向查找指定字符，遇到停止符或指定的字符串则返回失败
	int findStrForward(unsigned char *buffer, int bufferSize, unsigned char *leftPos, unsigned char *&rightPos); //前向查找字符串，C++ 函数名和变量命名规则，数字 + 字母 + 下划线
	int findStrBack(unsigned char *buffer, int bufferSize, unsigned char *rightPos, unsigned char *&leftPos); //反向查找字符串，C++ 函数名和变量命名规则，数字 + 字母 + 下划线
	int findStrVarTypeBack(unsigned char *buffer, int bufferSize, unsigned char *rightPos, unsigned char *&leftPos); //反向查找变量类型字符串，C++ 函数名和变量命名规则，数字 + 字母 + 下划线
	int findStrCharsBackGreedy(unsigned char *buffer, int bufferSize, unsigned char *rightPos, char *chars, unsigned char *&leftPos); //反向查找字符串或者指定的字符，贪婪查找直到找到为止，C++ 函数名和变量命名规则，数字 + 字母 + 下划线
	int findQueryStrBackStop(unsigned char *buffer, int bufferSize, unsigned char *rightPos, char *queryStr, char *stopChar, unsigned char *&leftPos); //反向查找指定字符串，遇到停止符则返回失败
	int findQueryStrForwardStop(unsigned char *buffer, int bufferSize, unsigned char *leftPos, char *queryStr, char *stopChar, unsigned char *&rightPos); //前向查找指定字符串，遇到停止符则返回失败
	int findVarDeclareForward(unsigned char *buffer, int bufferSize, std::string queryStr, std::string &varDeclareType); //前向查找变量声明的类型
	int findVarDeclareBack(unsigned char *buffer, int bufferSize, std::string queryStr, std::string &varDeclareType); //反向查找变量声明的类型
	bool isValidVarChar(char ch); //是否是一个有效的变量命名，C++ 函数名和变量命名规则，数字 + 字母 + 下划线
	bool isWhiteSpace(char ch); //是否是一个空白字符，' ', '\t', '\r', '\n'
	int replaceTwoMoreWhiteSpaceByOneSpace(unsigned char *buffer, int bufferSize); //将多个连续的空白字符用一个空格代替
	int statBufferLinesCount(unsigned char *buffer, int bufferSize); //统计给定内存中换行符'\n'的个数
	
	int macroExpand(); //将宏定义展开
	bool isParentClass(std::string child, std::string parent, std::vector<FUNCTIONS> &vFunctions); //判断parent类是否是child的父类
	int splitParentsClass(unsigned char *buffer, int bufferSize, std::vector<MY_STRING> &classParents); //拆分父类们，例如："class B : public A, public C{};"
	int updateParentClass(std::vector<FUNCTIONS> &vFunctions); //主要解决两层以上的父类继承
	int findAllMemberVarsInClassDeclare(unsigned char *buffer, int bufferSize, CLASS_STRUCT &classes, unsigned char *bufferBase, int lineNumberBase); //在类/结构体的声明语句块内部，提取出所有声明的成员变量
	int findAllMemberFuncsInClassDeclare(unsigned char *buffer, int bufferSize, CLASS_STRUCT &classes, unsigned char *bufferBase, int lineNumberBase); //在类/结构体的声明语句块内部，提取出所有声明的成员函数
	bool isFunctionArgsMatch(std::string parameter, std::string functionArgs); //函数的声明的参数列表和函数的调用传入的参数列表是否匹配
	int statAllFuns(std::vector<FUNCTIONS> &vFunctions); //统计所有函数之间的调用关系
	int createAllFunsCalledTree(std::vector<FUNCTIONS> &vFunctions, FUNCS_CALLED_TREE &trees); //创建函数调用关系多叉树
	int createFunsCalledTree(std::vector<FUNCTIONS> &vFunctions, int queryFuncIndex, FUNC_INDEX **&arry, int arryLen); //创建函数调用关系多叉树
	int getFuncsPos(std::vector<FUNCTIONS> &vFunctions, int queryFuncIndex, int &index1, int &index2); //创建函数调用关系多叉树
	int dumpBufferToFile(unsigned char *buffer, int bufferSize, char *filename); //将内存数据写到磁盘文件
	int printInfo(std::vector<FUNCTIONS> &vFunctions);
};

#endif //__FUNC_ROUTE_H__
