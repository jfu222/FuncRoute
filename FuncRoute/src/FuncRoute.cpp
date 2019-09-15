#include "FuncRoute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./os/share_library.h"
#include "CPPKeyword.h"
#include "CKeyword.h"


#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define ROUND(x) ((int) ((x) + 0.5))

#define RETURN_IF_FAILED(condition, ret)                                                      \
    do                                                                                        \
    {                                                                                         \
        if (condition)                                                                        \
        {                                                                                     \
            printf("%s(%d): %s: Error: ret=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret);    \
            return ret;                                                                       \
        }                                                                                     \
    } while (0)


//#define A(x)    #@x         //对单字符加单引号,例如：A(x) 表示 'x',A(abcd)则无效
//#define B(x)    #x          //加双引号，即将x转换成字符串，例如：B(hello)，表示 "hello"
//#define C(x)    hello##x    //把标识符连接起来，犹如胶水一样，将两个单词粘起来，例如：C(_world)，表示hello_world


//-----------------------------
CFuncRoute::CFuncRoute()
{

}


CFuncRoute::~CFuncRoute()
{

}


int CFuncRoute::findAllFunctionsName(std::string filePath, std::vector<std::string> suffixes)
{
	int ret = 0;
	std::vector<std::string> files;
	std::vector<std::string> files2;

	ret = get_nested_dir_files(filePath.c_str(), files);
	RETURN_IF_FAILED(ret != 0, ret);

	//---------------------------
	int len1 = files.size();
	RETURN_IF_FAILED(len1 <= 0, -1);

	int len2 = suffixes.size();
	RETURN_IF_FAILED(len2 <= 0, -2);

	bool isShouldFilterFiles = true; //是否需要按照后缀名过滤文件名
	for (int j = 0; j < len2; ++j)
	{
		if (suffixes[j] == "*") //星号"*"表示匹配所有文件
		{
			isShouldFilterFiles = false;
			break;
		}
	}

	if (isShouldFilterFiles == true)
	{
		for (int i = 0; i < len1; ++i)
		{
			for (int j = 0; j < len2; ++j)
			{
				if (suffixes[j] != "*")
				{
					int len21 = files[i].length();
					int len22 = suffixes[j].length();

					std::size_t pos = files[i].rfind(".");
					if (pos != std::string::npos)
					{
						if (files[i].substr(pos) == suffixes[j])
						{
							files2.push_back(files[i]);
						}
					}
				}
			}
		}
	}
	else
	{
		files2 = files;
	}

	//------------------------
	int len3 = files2.size();
	RETURN_IF_FAILED(len3 <= 0, -3);
	
	//---------将所有文件里面宏定义提取出来---------------
	std::vector<MACRO> macros;
	ret = findAllMacros(files2, macros);
	RETURN_IF_FAILED(ret, -3);

	//---------提取每个文件里面的函数定义-------------------
	std::vector<FUNCTIONS> allFuncs;

	for (int i = 0; i < len3; ++i)
	{
		printf("%s: %s\n", __FUNCTION__, files2[i].c_str());

		//-----------读取整个文件到内存中---------------
		FILE * fp = fopen(files2[i].c_str(), "rb");
		RETURN_IF_FAILED(fp == NULL, -4);

		fseek(fp, 0, SEEK_END);
		long file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char)* file_size);
		RETURN_IF_FAILED(buffer == NULL, -5);

		size_t read_size = fread(buffer, file_size, 1, fp);
		if (read_size != 1)
		{
			printf("%s: Error: read_size=%d != 1\n", __FUNCTION__, read_size);
			fclose(fp);
			free(buffer);
			return -6;
		}

		fclose(fp);

		//---------------------
		FUNCTIONS functions;
		memset(&functions, 0, sizeof(FUNCTIONS));

		int len = MIN(files2[i].length(), sizeof(functions.fllename) - 1);

		memcpy(functions.fllename, files2[i].c_str(), len);
		functions.fllename[len] = '\0';

		ret = search_CPP_FuncName(buffer, file_size, functions);
		free(buffer);

		int len2 = functions.funcs.size();
		if (ret != 0 && len2 <= 0)
		{
			printf("[%d] %s; Warn: can not find any functions;\n", i, functions.fllename);
			continue;
		}

		allFuncs.push_back(functions);

		//-----------------
		for (int i = 0; i < len2; ++i)
		{
			printf("[%d/%d] %s; line=%d;\n", i + 1, len2, functions.funcs[i].funcString, functions.funcs[i].functionName.lineNumberOfStart);
		}
	}

	//---------分析各个函数之间的调用关系--------------
	ret = statAllFuns(allFuncs);

	return ret;
}


int CFuncRoute::search_C_FuncName(unsigned char *buffer, unsigned int bufferSize, FUNCTIONS &functions)
{
	int ret = 0;

	return 0;
}


int CFuncRoute::search_CPP_FuncName(unsigned char *buffer, unsigned int bufferSize, FUNCTIONS &functions)
{
	int ret = 0;

	RETURN_IF_FAILED(buffer == NULL, -1);

	char whiteSpace[] = {' ', '\t', '\r', '\n'}; //空白字符
	char scopeResolutionOperator[] = "::"; //C++ 作用域限定符
	char varName[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"; //C++ 变量命名规则，数字 + 字母 + 下划线

	FUNCTION_STRUCTURE funcStruct;

	memset(&funcStruct, 0, sizeof(FUNCTION_STRUCTURE));

	unsigned char *buffer2 = (unsigned char *)malloc(bufferSize);
	RETURN_IF_FAILED(buffer2 == NULL, -2);

	memcpy(buffer2, buffer, bufferSize); //复制一份

	unsigned char *p1 = buffer2;
	unsigned char *p2 = buffer2;
	unsigned char *p3 = buffer2 + bufferSize - 1;
	unsigned char *p11 = NULL;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	unsigned char *p23 = NULL;
	int lineNumber = 1;
	int lineNumberTemp = 1;
	int braceCount = 0; //大括号对计数
	int parenthesesCount = 0; //小括号对计数
	int lenFuncString = sizeof(funcStruct.funcString);
	bool ret2 = false;

	int wordCountMax = 5; //函数名前面，最多搜索5个以空格隔开的字符串
	int wordCount = 0;
	int overSerach = 0; //停止搜索
	int lineCntMax = 5; //函数名前面，最多搜索5行
	int lineCnt = 0;
	int len = 0;

	//------先将被注释掉的代码用空格' '代替（行号符'\n'保留）--------
	ret = replaceAllCodeCommentsBySpace(buffer2, bufferSize);
	RETURN_IF_FAILED(ret, -2);
	
//	static int cnt = 0;
//	char file22[600] = "";
//	sprintf(file22, "./out%d.txt", cnt++);
//	ret = dumpBufferToFile(buffer2, bufferSize, file22);

	//------将所有用双引号""的代码用空格' '代替--------
	ret = replaceAllStrBySpace(buffer2, bufferSize);
	RETURN_IF_FAILED(ret, -2);

//	sprintf(file22, "./out%d.txt", cnt++);
//	ret = dumpBufferToFile(buffer2, bufferSize, file22);

	//------查找所有的C++类名(关键字class, struct)--------
	char classKeyword[] = "class";
	char structKeyword[] = "struct";
	int classLen = strlen(classKeyword);
	int structLen = strlen(structKeyword);
	bool isStruct = false;
	bool isDestructor = false; //是否是析构函数
	bool isConstructorArgs = false; //是否是带参数列表的构造函数
	bool isFuncNameWhithClassName = false; //函数定义是否带类名，类似 int A::get(){}

	//--------查找关键字class/struct----------------
	ret = findAllClassAndStructDeclare(p1, p3 - p1 + 1, functions.classes);

	//---------------------------
	p2 = buffer2;
	p21 = p2;
	p22 = NULL;

retry:
	while (p21 <= p3)
	{
		memset(&funcStruct, 0, sizeof(FUNCTION_STRUCTURE));

		//--------查找函数体左右大括号"{}"----------------
		ret = findCharForward(p21, p3 - p21 + 1, '{', p22);
		if (ret != 0)
		{
			break;
		}

		lineNumber += statBufferLinesCount(p21, p22 - p21 + 1);
		
		funcStruct.functionBody.start = p22;
		funcStruct.functionBody.fileOffsetOfStart = funcStruct.functionBody.start - p1;
		funcStruct.functionBody.lineNumberOfStart = lineNumber;

		ret = findPairCharForward(p22, p3 - p21 + 1, p22, '{', '}', p22);
		if (ret != 0)
		{
			break;
		}

		funcStruct.functionBody.end = p22;
		funcStruct.functionBody.fileOffsetOfEnd = funcStruct.functionBody.end - p1;
		lineNumber += statBufferLinesCount(funcStruct.functionBody.start, funcStruct.functionBody.end - funcStruct.functionBody.start + 1);
		funcStruct.functionBody.lineNumberOfEnd = lineNumber;

		//---检查大括号对语句块是否是宏定义----
		ret = findQueryStrBackStop(p1, funcStruct.functionBody.start - p1 + 1, funcStruct.functionBody.start, "#", "\n", p21);
		if (ret == 0)
		{
			goto retry4; //说明是单行宏定义，则重新查找
		}
		
		p22 = funcStruct.functionBody.start;
		ret = findCharForwardStop(p22, funcStruct.functionBody.end - p22 + 1, '\\', "\n", p21);
		if (ret == 0)
		{
			goto retry4; //可能是类似 "#define AAAA \\ " 的声明，而不是函数定义，则跳过
		}

		//--------查找函数参数左右小括号对"()"----------------
		p21 = funcStruct.functionBody.start - 1;

		lineNumber = funcStruct.functionBody.lineNumberOfStart;
		ret = skipWhiteSpaceBack(p1, funcStruct.functionBody.start - p21 + 1, p21, p21, lineNumber);
		RETURN_IF_FAILED(ret, ret);
		
		ret = findCharBackStop(p1, p21 - p1 + 1, ')', "{}", p21);
		if (ret != 0)
		{
			goto retry4; //未找到函数参数右小括号')'
		}

retry0:
		funcStruct.functionParameter.end = p21;
		funcStruct.functionParameter.fileOffsetOfEnd = funcStruct.functionParameter.end - p1;
		funcStruct.functionParameter.lineNumberOfEnd = lineNumber;

		ret = findPairCharBackStop(p1, p21 - p1 + 1, p21, '(', ')', ";{}", p21);
		if (ret != 0)
		{
			goto retry4; //可能是类似 "struct A {};" 的声明，而不是函数定义，则跳过
		}

		funcStruct.functionParameter.start = p21;
		funcStruct.functionParameter.fileOffsetOfStart = funcStruct.functionParameter.end - p1;

		lineNumber -= statBufferLinesCount(funcStruct.functionParameter.start, funcStruct.functionParameter.length);
		funcStruct.functionParameter.lineNumberOfStart = lineNumber;

		//-----尝试查找函数参数列表右小括号后面的C/C++修饰符(type-qualifier类型限定符)--------
		p21 = funcStruct.functionParameter.end + 1;
		lineNumber = funcStruct.functionParameter.lineNumberOfEnd;

		ret = skipWhiteSpaceForward(p21, funcStruct.functionBody.start - p21 + 1, p21, p21, lineNumber);
		RETURN_IF_FAILED(ret, ret);

		funcStruct.functionTypeQualifier.start = p21;
		funcStruct.functionTypeQualifier.fileOffsetOfStart = funcStruct.functionTypeQualifier.start - p1;
		funcStruct.functionTypeQualifier.lineNumberOfStart = lineNumber;

		p22 = funcStruct.functionBody.start - 1;
		ret = skipWhiteSpaceBack(p21, p22 - p21 + 1, p22, p22, lineNumber);
		if (ret != 0)
		{
			goto retry1; //说明没有修饰符，则跳过
		}

		funcStruct.functionTypeQualifier.end = p22;
		funcStruct.functionTypeQualifier.fileOffsetOfEnd = funcStruct.functionTypeQualifier.end - p1;
		funcStruct.functionTypeQualifier.lineNumberOfEnd = lineNumber;

		if(funcStruct.functionTypeQualifier.end - funcStruct.functionTypeQualifier.start + 1 > 1)
		{
			ret = findCharForward(funcStruct.functionTypeQualifier.start, funcStruct.functionTypeQualifier.end - funcStruct.functionTypeQualifier.start + 1, '#', p22);
			if(ret == 0) //FIXME: 目前只支持 类似 int get() const { return 0; }
			{
				goto retry4;
			}
		}

		if (funcStruct.functionTypeQualifier.start == funcStruct.functionTypeQualifier.end)
		{
			memset(&funcStruct.functionTypeQualifier, 0, sizeof(funcStruct.functionTypeQualifier));
		}

retry1:
		//--------查找函数名----------------
		lineNumber = funcStruct.functionParameter.lineNumberOfStart;
		p21 = funcStruct.functionParameter.start - 1;

		ret = skipWhiteSpaceBack(p1, p21 - p1 + 1, p21, p21, lineNumber);
		RETURN_IF_FAILED(ret, ret);

		funcStruct.functionName.end = p21;
		funcStruct.functionName.fileOffsetOfEnd = funcStruct.functionName.end - p1;
		funcStruct.functionName.lineNumberOfEnd = lineNumber;

		ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
		RETURN_IF_FAILED(ret, ret);

		//---反向尝试查找C++ 类作用域限定符"::"------
		ret = findQueryStrBackStop(p1, p21 - p1 + 1, p21, "::", ";{}()", p21); //类似 int A::B::get() { return 0; }
		if(ret == 0)
		{
			p21--;

			ret = skipWhiteSpaceBack(p1, p21 - p1 + 1, p21, p21, lineNumber);
			RETURN_IF_FAILED(ret, ret);

			ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
			RETURN_IF_FAILED(ret, ret);

			isFuncNameWhithClassName = true; //函数定义是否带类名，类似 int A::get(){}
		}

		ret = findCharBackStop(p1, p21 - p1 + 1, '~', ";{}()", p21); //尝试查找析构函数，类似 ~A::A(){}
		if (ret == 0)
		{
			isDestructor = true;
		}

		funcStruct.functionName.start = p21;
		funcStruct.functionName.fileOffsetOfStart = funcStruct.functionName.start - p1;
		funcStruct.functionName.lineNumberOfStart = lineNumber;
		
		//---判断是否是带初始化列表的构造函数，类似 "class A {public: int m_a; int m_b; public: A::A():m_a(1),m_b(0){}};"
		if(isFuncNameWhithClassName == false)
		{
			ret = findCharBackStop(p1, p21 - p1 + 1, ':', ";{}", p21);
			if (ret == 0)
			{
				p21--;
				ret = skipWhiteSpaceBack(p1, p21 - p1 + 1, p21, p21, lineNumber);
				RETURN_IF_FAILED(ret, ret);

				if(*p21 == ')') //说明是带初始化列表的构造函数
				{
					goto retry0;
				}
			}
		}

		//-----检查函数名是否是C++关键词，例如 if (a == 1) {} --------
		ret2 = isKeyword(funcStruct.functionName.start, funcStruct.functionName.end - funcStruct.functionName.start + 1);
		if (ret2) //函数名是C/C++语言关键词
		{
			goto retry4;
		}

		//--------查找函返回值----------------
		if (isDestructor == true)
		{
			goto retry3; //析构函数没有返回值
		}

		p21 = funcStruct.functionName.start - 1;

		ret = skipWhiteSpaceBack(p1, p21 - p1 + 1, p21, p21, lineNumber);
		if (ret != 0)
		{
			goto retry3; //像构造函数和析构函数就没有返回值
		}

		funcStruct.functionReturnValue.end = p21;
		funcStruct.functionReturnValue.fileOffsetOfEnd = funcStruct.functionReturnValue.end - p1;
		funcStruct.functionReturnValue.lineNumberOfEnd = lineNumber;

		if(*p21 == '*') //函数返回值类型是一个地址指针
		{
			p21--;
		}

		// 函数返回值(type-qualifier类型限定符)： const，template, virtual, inline, static, extern, explicit, friend, constexpr
		// 函数返回值类型(type - specifier类型区分符) : void *, void, int, short, long, float, double, auto, struct结构体类型，enum枚举类型，typedef类型, uint32_t
		// 比如: inline const unsigned long long * get(int &a) const { return 0; }
		
		wordCountMax = 5; //函数名前面，最多搜索5个以空格隔开的字符串
		wordCount = 0;
		overSerach = 0; //停止搜索
		lineCntMax = 5; //函数名前面，最多搜索5行
		lineCnt = 0;
		p22 = p21; //保存p21值

retry2:
		ret = skipWhiteSpaceBack(p1, p21 - p1 + 1, p21, p21, lineNumber);

		while (p21 > p1)
		{
			if ((*p21 >= '0' && *p21 <= '9')
				|| (*p21 >= 'a' && *p21 <= 'z')
				|| (*p21 >= 'A' && *p21 <= 'Z')
				|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
				|| (*p21 == '<' || *p21 == '>') //C++ 模板 template <typename T> class A {};
				|| (*p21 == '\r' || *p21 == '\n') //返回值分散在多行
				)
			{

			}
			else
			{
				if (p21 == p1 //搜到起始位置了
					|| *p21 == '{' //可能函数体在C++类的内部
					|| *p21 == '}' //可能是上一个函数结束的位置
					|| *p21 == ';' //语句结束
					|| *p21 == ')' //类似 #pragma comment(lib, "user32.lib")
					|| *p21 == '\\' //类似 '#define  MACRO_A \'
					|| *p21 == ':' //类似 class A { public: A(){} };
					|| *p21 == '"' //类似 '#include "FuncRoute.h" int main() { return 0; };'
					|| *p21 == '`' //特殊字符，不属于C++规范，仅作为本解析器替换字符用
					)
				{
					overSerach = 1; //停止搜索

					p21++;
					while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
					{
						if (*p21 == '\n')
						{
							lineNumber++;
						}
						p21++;
					}
				}
				else if (*p21 == '\n')
				{
					lineCnt++;
				}
				break;
			}

			p21--;
		}

		if (p21 < p1)
		{
			ret = -7;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		wordCount++;
		if (p21 != p1 && overSerach == 0 && wordCount < wordCountMax && lineCnt < lineCntMax)
		{
			goto retry2;
		}

		funcStruct.functionReturnValue.start = p21;
		funcStruct.functionReturnValue.fileOffsetOfStart = funcStruct.functionReturnValue.start - p1;
		funcStruct.functionReturnValue.lineNumberOfStart = lineNumber;

		funcStruct.functionReturnValue.length = funcStruct.functionReturnValue.end - funcStruct.functionReturnValue.start + 1;
		funcStruct.functionReturnValue.copyStrFromBuffer();

retry3:
		//--------查找到一个完整的函数----------------
		funcStruct.functionReturnValue.length = (funcStruct.functionReturnValue.end && funcStruct.functionReturnValue.start) ? (funcStruct.functionReturnValue.end - funcStruct.functionReturnValue.start + 1) : 0;
		funcStruct.functionName.length = (funcStruct.functionName.end && funcStruct.functionName.start) ? (funcStruct.functionName.end - funcStruct.functionName.start + 1) : 0;
		funcStruct.functionParameter.length = (funcStruct.functionParameter.end && funcStruct.functionParameter.start) ? (funcStruct.functionParameter.end - funcStruct.functionParameter.start + 1) : 0;
		funcStruct.functionTypeQualifier.length = (funcStruct.functionTypeQualifier.end && funcStruct.functionTypeQualifier.start) ? (funcStruct.functionTypeQualifier.end - funcStruct.functionTypeQualifier.start + 1) : 0;
		funcStruct.functionBody.length = (funcStruct.functionBody.end && funcStruct.functionBody.start) ? (funcStruct.functionBody.end - funcStruct.functionBody.start + 1) : 0;

		funcStruct.functionReturnValue.copyStrFromBuffer();
		funcStruct.functionName.copyStrFromBuffer();
		funcStruct.functionParameter.copyStrFromBuffer();
		funcStruct.functionTypeQualifier.copyStrFromBuffer();
		funcStruct.functionBody.copyStrFromBuffer();

		if (funcStruct.functionReturnValue.length + 1 + funcStruct.functionName.length + 1 + funcStruct.functionParameter.length + 1 + funcStruct.functionTypeQualifier.length < lenFuncString)
		{
			char * pTemp = funcStruct.funcString;
			int flag= 0;

			if (funcStruct.functionReturnValue.length > 0)
			{
				memcpy(pTemp, funcStruct.functionReturnValue.start, funcStruct.functionReturnValue.length);
				pTemp += funcStruct.functionReturnValue.length;
				flag = 1;
			}

			if (funcStruct.functionName.length > 0)
			{
				if(flag != 0){ *pTemp = ' '; pTemp++; }
				memcpy(pTemp, funcStruct.functionName.start, funcStruct.functionName.length);
				pTemp += funcStruct.functionName.length;
				flag = 2;
			}

			if (funcStruct.functionParameter.length > 0)
			{
				if(flag != 0){ *pTemp = ' '; pTemp++; }
				memcpy(pTemp, funcStruct.functionParameter.start, funcStruct.functionParameter.length);
				pTemp += funcStruct.functionParameter.length;
				flag = 3;
			}

			if (funcStruct.functionTypeQualifier.length > 0)
			{
				if(flag != 0){ *pTemp = ' '; pTemp++; }
				memcpy(pTemp, funcStruct.functionTypeQualifier.start, funcStruct.functionTypeQualifier.length);
				pTemp += funcStruct.functionTypeQualifier.length;
				flag = 4;
			}
			*pTemp = '\0';
		}
		else
		{
			ret = -9;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}
		
		//--------查找函数体内部调用了哪些其他函数---------------
		ret = findAllFuncsInFunctionBody(funcStruct.functionBody.start, funcStruct.functionBody.length, funcStruct.funcsWhichInFunctionBody, p1, funcStruct.functionBody.lineNumberOfStart);
		if (ret != 0)
		{
			ret = -8;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		//------确定函数是哪一个C++类的成员函数-----------
		for (int i = 0; i < functions.classes.size(); ++i)
		{
			if (funcStruct.functionName.start > functions.classes[i].classBody.start && funcStruct.functionName.end < functions.classes[i].classBody.end)
			{
				if (functions.classes[i].isStruct) //说明是结构体的成员函数
				{
					int len = MIN(functions.classes[i].className.length, sizeof(funcStruct.className) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.structName, functions.classes[i].className.str, len);
						funcStruct.structName[len] = '\0';
					}

					len = MIN(functions.classes[i].classNameAlias.length, sizeof(funcStruct.classNameAlias) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.classNameAlias, functions.classes[i].classNameAlias.str, len);
						funcStruct.classNameAlias[len] = '\0';
					}
				}else //说明是C++类的成员函数
				{
					int len = MIN(functions.classes[i].className.length, sizeof(funcStruct.className) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.className, functions.classes[i].className.str, len);
						funcStruct.className[len] = '\0';
					}
				}
			}
		}
		
		//------从函数名中，提取函数类名-----------
		if(strlen(funcStruct.className) <= 0)
		{
			for(int i = 0; i < (funcStruct.functionName.length - 2); ++i)
			{
				if(memcmp(funcStruct.functionName.str + i, "::", 2) == 0) //将 "B::set(int a)"中的"B"提取出来
				{
					int len = MIN(i, sizeof(funcStruct.className) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.className, funcStruct.functionName.str, len);
						funcStruct.className[len] = '\0';
					}
				}
			}
		}

		//------将结果保存起来----------
		functions.funcs.push_back(funcStruct);

retry4:
		//-------继续下一轮循环---------------
		lineNumber = funcStruct.functionBody.lineNumberOfStart;
		p21 = funcStruct.functionBody.start + 1;
		if (*p21 == '\n')
		{
			lineNumber++;
		}
		p1 = p21; //更新 p1 的值
	}

end:
	//----------------
	if (buffer2){ free(buffer2); buffer2 = NULL; }

	return 0;
}


bool CFuncRoute::isKeyword(unsigned char *buffer, int bufferSize)
{
	if(buffer == NULL || bufferSize <= 0)
	{
		return false;
	}

	int len1 = sizeof(cpp_keywords) /  sizeof(cpp_keywords[0]);
	int len2 = sizeof(cpp_preprocessors) /  sizeof(cpp_preprocessors[0]);

	for(int i = 0; i < len1; ++i)
	{
		int strLen = strlen(cpp_keywords[i]);
		if(bufferSize == strLen)
		{
			if(memcmp(buffer, cpp_keywords[i], strLen) == 0)
			{
				return true;
			}
		}
	}
	
	for(int i = 0; i < len2; ++i)
	{
		int strLen = strlen(cpp_preprocessors[i]);
		if(bufferSize == strLen)
		{
			if(memcmp(buffer, cpp_preprocessors[i], strLen) == 0)
			{
				return true;
			}
		}
	}

	return false;
}


int  CFuncRoute::replaceAllCodeCommentsBySpace(unsigned char *buffer, int bufferSize)
{
	int ret = 0;
	
	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;

	//------先将被注释掉的代码用空格' '代替（换行号符'\n'保留）--------
	while (p2 <= p3)
	{
		if (*p2 == '/') //被斜线(oblique line)"/"注释掉的字符串
		{
			p21 = p2;

			if (p2 + 1 <= p3 && *(p2 + 1) == '*') //说明是用 "/*...*/" 进行的多行代码注释
			{
				p2 += 2;

				while (p2 <= p3)
				{
					if (*p2 == '*' && p2 + 1 <= p3 && *(p2 + 1) == '/') //说明找到了"*/"
					{
						p2++;
						break;
					}
					p2++;
				}

				if (p2 <= p3)
				{
					for (; p21 <= p2; ++p21)
					{
						if (*p21 != '\n') //换行符不覆盖
						{
							*p21 = ' '; //用空格' '覆盖原有的字符
						}
					}
				}
			}
			else if (p2 + 1 <= p3 && *(p2 + 1) == '/') //说明是用 "//..." 进行的单行代码注释
			{
				while (p2 <= p3)
				{
					if (*p2 != '\n') //换行符不覆盖
					{
						*p2 = ' '; //用空格' '覆盖原有的字符
					}
					else
					{
						break; //继续查找下一处注释
					}
					p2++;
				}
			}
			else
			{
				//do nothing
			}
		}

		p2++;
	}

	return 0;
}


int  CFuncRoute::replaceAllStrBySpace(unsigned char *buffer, int bufferSize)
{
	int ret = 0;
	
	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;

	//------先将两个连续的反斜杠"\\"用空格' '代替--------
	while (p2 <= p3 - 1)
	{
		if (*p2 == '\\' && *(p2 + 1) == '\\') //两个连续的"\\"
		{
			*p2 = '`';
			p2++;
			*p2 = '`';
		}
		p2++;
	}
	
	//------再将反斜杠"\w"转义字符用空格' '代替--------
	p2 = p1;
	while (p2 <= p3 - 1)
	{
		if (*p2 == '\\' && (*(p2 + 1) == '"' || *(p2 + 1) == '\''))
		{
			*p2 = '`';
			p2++;
			*p2 = '`';
		}
		p2++;
	}

	//------将单引号引'起来的代码用空格' '代替（换行号符'\n'保留）--------
	int flag = 0;
	p2 = p1;
	while (p2 <= p3)
	{
		if (*p2 == '\'')
		{
			if (flag == 0)
			{
				flag = 1;
				p21 = p2;
			}
			else if (flag == 1)
			{
				flag = 0;
				while (p21 <= p2)
				{
					*p21 = '`'; //用空格' '代替
					p21++;
				}
			}
		}
		p2++;
	}

	//------将双引号引""起来的代码用空格' '代替（换行号符'\n'保留）--------
	flag = 0;
	p2 = p1;
	while (p2 <= p3)
	{
		if (*p2 == '"')
		{
			if(flag == 0)
			{
				flag = 1;
				p21 = p2;
			}else if(flag == 1)
			{
				flag = 0;
				while (p21 <= p2)
				{
					*p21 = '`'; //用空格' '代替
					p21++;
				}
			}
		}
		p2++;
	}
	
	return 0;
}


int CFuncRoute::findStr(unsigned char *buffer, int bufferSize, const char *str, int &pos)
{
	RETURN_IF_FAILED(buffer == NULL || bufferSize <= 0 || str == NULL, -1);
	
	int strLen = strlen(str);
	RETURN_IF_FAILED(strLen <= 0, -2);

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1 - strLen;
	unsigned char *p21 = NULL;

	while(p2 <= p3)
	{
		if(memcmp(p2, str, strLen) == 0) //found it
		{
			pos = p2 - p1;
			return 0;
		}
		p2++;
	}

	return -1;
}


int CFuncRoute::findAllMacros(std::vector<std::string> files, std::vector<MACRO> &macros)
{
	int ret = 0;

	int len = files.size();
	RETURN_IF_FAILED(len <= 0, -1);

	for (int i = 0; i < len; ++i)
	{
		printf("%s: %s\n", __FUNCTION__, files[i].c_str());

		//-----------读取整个文件到内存中---------------
		FILE * fp = fopen(files[i].c_str(), "rb");
		RETURN_IF_FAILED(fp == NULL, -2);

		fseek(fp, 0, SEEK_END);
		long file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char)* file_size);
		RETURN_IF_FAILED(buffer == NULL, -3);

		size_t read_size = fread(buffer, file_size, 1, fp);
		if (read_size != 1)
		{
			printf("%s: Error: read_size=%d != 1\n", __FUNCTION__, read_size);
			fclose(fp);
			free(buffer);
			return -6;
		}

		fclose(fp);

		//-------将所有用"//..."或"/*...*/"注释掉的代码用空格' '代替----------------
		ret = replaceAllCodeCommentsBySpace(buffer, file_size);
		RETURN_IF_FAILED(ret, -4);

		//-----------------------
		char define[] = "#define";
		int defineLen = strlen(define);

		unsigned char *p1 = buffer;
		unsigned char *p2 = buffer;
		unsigned char *p3 = buffer + file_size - 1 - defineLen;
		unsigned char *p21 = NULL;
		unsigned char *p22 = NULL;
		int lineNumber = 1;

		while(p2 <= p3)
		{
			if(memcmp(p2, define, defineLen) == 0) //found it
			{
				p2 += defineLen;
				p21 = p2;
				while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
				{
					if (*p21 == '\n')
					{
						lineNumber++;
					}
					p21++;
				}

				if (p21 <= p3 && p21 > p2) //宏定义  "#define PI(a, b)    3.1415926"中，"#define"和"PI(a, b)"之间必须至少有一个空白字符
				{
					p22 = p21;
					while (p21 <= p3)
					{
						if (!((*p21 >= '0' && *p21 <= '9')
							|| (*p21 >= 'a' && *p21 <= 'z')
							|| (*p21 >= 'A' && *p21 <= 'Z')
							|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
							))
						{
							break;
						}

						p21++;
					}

					if(p21 <= p3) //找到 "#define PI(a, b)    3.1415926" 中的 宏名"PI(a, b)"
					{
						MACRO macro;
						memset(&macro, 0, sizeof(MACRO));

						int len33 = MIN(p21 - p22, sizeof(macro.macroName) - 1);
						if(len33 > 0)
						{
							memcpy(macro.macroName, p22, len33);
							macro.macroName[len33] = '\0';

							//-------检查宏定义的参数列表，即"#define PI(a, b)    3.1415926" 中的"(a, b)"---------------
							if(*p21 == '(')
							{
								p22 = p21;
								while (p21 <= p3)
								{
									if(*p21 == ')')
									{
										break;
									}
									if (*p21 == '\r'|| *p21 == '\n') //宏名后面以一个空白字符结束
									{
										*p21 = ' '; //用空格替换
									}

									p21++;
								}

								if(p21 <= p3) //找到宏名参数列表了
								{
									int macroArgsLen = p21 - p22 + 1;
									if(macroArgsLen > sizeof(macro.macroArgs) - 1)
									{
										ret = -6;
										printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
										break;
									}
									
									int len33 = MIN(macroArgsLen, sizeof(macro.macroName) - 1);
									if(len33 > 0)
									{
										memcpy(macro.macroArgs, p22, len33);
										macro.macroArgs[len33] = '\0';
										p21++;
									}
								}else
								{
									ret = -6;
									printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
									break;
								}
							}
							//--------查找宏体-------------
							p22 = p21;
							while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t')) //跳过空白字符
							{
								p21++;
							}

							if(p21 > p22) //宏名和宏体之间必须至少有一个空格，即"#define PI(a, b)    3.1415926"中"PI(a, b)"和"3.1415926"之间必须至少有一个空格
							{
								//-------多行宏定义时，每行最后一个字符必须是'\'反斜杠---------
								p22 = p21;
								while (p21 <= p3 + 3)
								{
									if(memcmp(p21, "\\\r\n", 3) == 0) //for windows
									{
										memset(p21, ' ', 3); //用空格替换
										p21 += 3;
									}
									else if(memcmp(p21, "\\\n", 2) == 0) //for linux
									{
										memset(p21, ' ', 2); //用空格替换
										p21 += 2;
									}else if(*p21 == '\n') //宏定义的最后一行换行符
									{
										break;
									}

									p21++;
								}

								if(p21 > p22) //说明找到宏体了
								{
									//-----将宏体中的连续多个空白字符替换成一个空白字符-------
									int macroBodyLen = p21 - p22 + 1;
									if(macroBodyLen > sizeof(macro.macroBody) - 1)
									{
										ret = -6;
										printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
										break;
									}
									char * p41 = macro.macroBody;

									while(p22 <= p21)
									{
										if(*p22 == ' ' || *p22 == '\t' || *p22 == '\r' || *p22 == '\n')
										{
											*p41 = ' '; //用一个空格代替
										}
										else
										{
											*p41 = *p22;
											p41++;
										}
										p22++;
									}
									*p41 = '\0';

									//-------查找到一个完整的宏定义----------
									macros.push_back(macro); //注意：有的宏定义里面还有宏定义，即宏定义嵌套，需要在后面再次展开宏定义
								}
								else
								{
									ret = -6;
									printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
									break;
								}
							}
							else //像 "#define __C_KEYWORD_H__" 也是合法的
							{
								ret = 0;
								printf("%s(%d): %s: Warn: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
//								break;
							}
						}else
						{
							ret = -6;
							printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
							break;
						}
					}
					else
					{
						ret = -6;
						printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
						break;
					}
				}
				else //像 char define[] = "#define"; 也是合法的
				{
					ret = 0;
					printf("%s(%d): %s: Warn: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
//					break;
				}
			}
			p2++;
		}
	}

	//-----展开嵌套的宏---------
	// ...
	
	//-----过滤#ifdef等条件宏---------
	// ...

	return ret;
}


int CFuncRoute::findAllClassAndStructDeclare(unsigned char *buffer, int bufferSize, std::vector<CLASS_STRUCT> &classes)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	unsigned char *p23 = NULL;
	int lineNumber = 1;

	//------查找所有的C++类名(关键字class, struct)--------
	char classKeyword[] = "class";
	char structKeyword[] = "struct";
	int classLen = strlen(classKeyword);
	int structLen = strlen(structKeyword);
	bool isStruct = false;

	//--------查找关键字class/struct----------------
	p21 = p2;
	while (p21 <= p3 - structLen)
	{
		isStruct = false;

		if (memcmp(p21, classKeyword, classLen) == 0 || memcmp(p21, structKeyword, structLen) == 0)
		{
			int lenKeyword = classLen;
			if (memcmp(p21, structKeyword, structLen) == 0)
			{
				lenKeyword = structLen;
				isStruct = true;
			}

			if ((p21 - 1 >= p1 && isValidVarChar(*(p21 - 1)) == false) //关键字前面必须有一个空白字符
				&& (p21 + lenKeyword + 1 <= p3 && isWhiteSpace(*(p21 + lenKeyword)) == true) //关键字class/struct后面至少有一个空白字符
				)
			{
				p21 += lenKeyword + 1;

				ret = skipWhiteSpaceForward(p21, p3 - p21 + 1, p21, p21, lineNumber);
				RETURN_IF_FAILED(ret, ret);

				//-----向前查找C++类名-----
				CLASS_STRUCT cs;
				memset(&cs, 0, sizeof(CLASS_STRUCT));
				cs.isStruct = isStruct;

				cs.className.start = p21;
				cs.className.fileOffsetOfStart = cs.className.start - p1;
				cs.className.lineNumberOfStart = lineNumber;

				ret = findStrForward(p21, p3 - p21 + 1, p21, p21);
				RETURN_IF_FAILED(ret, ret);

				cs.className.end = p21;
				cs.className.fileOffsetOfEnd = cs.className.end - p1;
				cs.className.lineNumberOfEnd = lineNumber;

				cs.className.length = cs.className.end - cs.className.start + 1;
				cs.className.copyStrFromBuffer();

				//-------向前查找C++类/结构体声明体----------
				ret = findCharForward(p21, p3 - p21 + 1, '{', p22); //前向查找类声明体语句块的起始字符'{'
				RETURN_IF_FAILED(ret, ret);

				ret = findCharForward(p21, p3 - p21 + 1, ';', p23); //前向查找类语句结束字符';'
				RETURN_IF_FAILED(ret, ret);

				if (p23 <= p22)
				{
					p21 = p23 + 1;
					continue; //跳过类似 class A; 保留 class A {...};
				}

				cs.classBody.start = p22;
				cs.classBody.fileOffsetOfStart = cs.classBody.start - p1;
				lineNumber += statBufferLinesCount(cs.className.end, cs.classBody.start - cs.className.end + 1);
				cs.classBody.lineNumberOfStart = lineNumber;

				ret = findPairCharForward(p22, p3 - p21 + 1, p22, '{', '}', p22);
				RETURN_IF_FAILED(ret, ret);

				cs.classBody.end = p22;
				cs.classBody.fileOffsetOfEnd = cs.classBody.end - p1;

				cs.classBody.length = cs.classBody.end - cs.classBody.start + 1;
				lineNumber += statBufferLinesCount(cs.classBody.start, cs.classBody.length);
				cs.classBody.lineNumberOfEnd = lineNumber;
				cs.classBody.copyStrFromBuffer();

				//-------检查类名和类声明体之间是否有继承的父类----------
				p21 = cs.className.end + 1;
				
				ret = findCharForward(p21, cs.classBody.start - p21 + 1, ':', p21);
				if (ret == 0) //说明有父类，类似："class B : public A, public C{};"
				{
					p21++;
					ret = skipWhiteSpaceForward(p21, cs.classBody.start - p21 + 1, p21, p21, lineNumber);
					RETURN_IF_FAILED(ret, ret);

					cs.classParent.start = p21;
					cs.classParent.fileOffsetOfStart = cs.classParent.start - p1;
					cs.classParent.lineNumberOfStart = lineNumber;

					ret = skipWhiteSpaceBack(p21, cs.classBody.start - p21 + 1, cs.classBody.start - 1, p21, lineNumber);
					RETURN_IF_FAILED(ret, ret);

					cs.classParent.end = p21;
					cs.classParent.fileOffsetOfEnd = cs.classParent.end - p1;
					cs.classParent.lineNumberOfEnd = lineNumber;

					cs.classParent.length = cs.classParent.end - cs.classParent.start + 1;
					cs.classParent.copyStrFromBuffer();

					//-------拆分父类："class B : public A, public C{};"------------
					ret = splitParentsClass(cs.classParent.start, cs.classParent.length, cs.classParents);
				}

				//--------尝试查找结构体的别名，类似 typedef struct _A_ {} A;---------------
				p21 = cs.classBody.end + 1;
				ret = findCharForward(p21, p3 - p21 + 1, ';', p22); //类声明必须以分号';'结束
				RETURN_IF_FAILED(ret, ret);

				ret = findCharForward(p21, p22 - p21 + 1, '{', p23);
				RETURN_IF_FAILED(ret == 0, ret); // "class A {};" 的 '}'和';'之间不能含有'{'

				if (isStruct)
				{
					ret = skipWhiteSpaceForward(p21, p22 - p21 + 1, p21, p21, lineNumber);
					RETURN_IF_FAILED(ret, ret);

					cs.classNameAlias.start = p21;
					cs.classNameAlias.fileOffsetOfStart = cs.classNameAlias.start - p1;
					cs.classNameAlias.lineNumberOfStart = lineNumber;

					ret = findStrForward(p21, p22 - p21 + 1, p21, p21);
					RETURN_IF_FAILED(ret, ret);

					cs.classNameAlias.end = p21;
					cs.classNameAlias.fileOffsetOfEnd = cs.classNameAlias.end - p1;
					cs.classNameAlias.lineNumberOfEnd = lineNumber;

					cs.classNameAlias.length = cs.classNameAlias.end - cs.classNameAlias.start + 1;
					cs.classNameAlias.copyStrFromBuffer();
				}

				//-------在类/结构体的声明语句块内部，提取出所有声明的成员变量---------
				ret = findAllMemberVarsInClassDeclare(cs.classBody.start, cs.classBody.length, cs, p1, cs.classBody.lineNumberOfStart);

				//-------在类/结构体的声明语句块内部，提取出所有声明的成员函数---------
				ret = findAllMemberFuncsInClassDeclare(cs.classBody.start, cs.classBody.length, cs, p1, cs.classBody.lineNumberOfStart);

				classes.push_back(cs); //找到一个完整的C++类/结构体声明
			}
		}

		if (*p21 == '\n')
		{
			lineNumber++;
		}

		p21++;
	}

	return 0;
}


int CFuncRoute::findAllFuncsInFunctionBody(unsigned char *buffer, int bufferSize, std::vector<CLASS_INSTANCE> &funcsWhichInFunctionBody, unsigned char *bufferBase, int lineNumberBase)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p11 = bufferBase;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int lineNumber = lineNumberBase;

	//--------查找函数体内部调用了哪些其他函数---------------
	while (p2 <= p3)
	{
		if (*p2 == '(') //可能是函数调用的参数列表的左括号
		{
			CLASS_INSTANCE ci;
			memset(&ci, 0, sizeof(CLASS_INSTANCE));

			ret = findWholeFuncCalled(p1, p3 - p1 + 1, p2, ci, bufferBase, lineNumber);
			if (ret == 0)
			{
				funcsWhichInFunctionBody.push_back(ci);
			}
		}
		
		if (*p2 == '\n')
		{
			lineNumber++;
		}

		p2++;
	}

end:

	return 0;
}


int CFuncRoute::findWholeFuncCalled(unsigned char *buffer, int bufferSize, unsigned char *parentheseLeft, CLASS_INSTANCE &classInstance, unsigned char *bufferBase, int lineNumberBase)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p11 = bufferBase;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int lineNumber = lineNumberBase;

	int flag = 0;

	p21 = parentheseLeft; //函数调用的参数列表左小括号位置

	//-------前向查找配对的右小括号------------
	classInstance.functionArgs.start = p21;
	classInstance.functionArgs.fileOffsetOfStart = p21 - p11;
	classInstance.functionArgs.lineNumberOfStart = lineNumber;

	ret = findPairCharForward(p21, p3 - p21 + 1, p21, '(', ')', p21);
	RETURN_IF_FAILED(ret, ret);

	classInstance.functionArgs.end = p21;
	classInstance.functionArgs.fileOffsetOfEnd = p21 - p11;
	classInstance.functionArgs.lineNumberOfEnd = lineNumber;

	classInstance.functionArgs.length = classInstance.functionArgs.end - classInstance.functionArgs.start + 1;
	classInstance.functionArgs.copyStrFromBuffer();

	//------查找左小括号前面的第一个非空白字符--------------
	p21 = parentheseLeft - 1;

	ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
	RETURN_IF_FAILED(ret, ret);

	//-------反向查找函数名------------
	classInstance.functionName.end = p21;
	classInstance.functionName.fileOffsetOfEnd = p21 - p11;
	classInstance.functionName.lineNumberOfEnd = lineNumber;

	if (*p21 == '>') //类似 std::TemplateA<std::string>(a, b); 模板函数调用
	{
		ret = findPairCharBackStop(p1, p21 - p1 + 1, p21, '<', '>', ";:{}", p21);
		RETURN_IF_FAILED(ret, ret);

		p21--;
		RETURN_IF_FAILED(p21 < p1, -1);
	}

	ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
//	RETURN_IF_FAILED(ret, ret);
	if (ret != 0) //类似 char * str = (char *)malloc(2);
	{
		return -1;
	}

	classInstance.functionName.start = p21;
	classInstance.functionName.fileOffsetOfStart = p21 - p11;
	classInstance.functionName.lineNumberOfStart = lineNumber;

	classInstance.functionName.length = classInstance.functionName.end - classInstance.functionName.start + 1;
	classInstance.functionName.copyStrFromBuffer();

	if (isKeyword((unsigned char *)classInstance.functionName.str, classInstance.functionName.length) == true) //类似 if( a == 2 ) 被当成函数调用了
	{
		return -1;
	}

	//-------反向查找类的实例调用函数的方式------------
	p21--;

	ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
	RETURN_IF_FAILED(ret, ret);

	classInstance.instanceType = VAR_TYPE_UNKNOWN; //可能是全局函数

	if (*p21 == '.') //使用点号"."调用函数，例如 ret = A.set(123);
	{
		classInstance.instanceType = VAR_TYPE_NORMAL;
	}else if (*p21 == '>') //使用箭头"->"调用函数，例如 ret = A->set(123);
	{
		RETURN_IF_FAILED(p21 - 1 < p1, -1);
		p21--;

		RETURN_IF_FAILED(*p21 != '-', -1);

		classInstance.instanceType = VAR_TYPE_POINTER;
	}
	else if (*p21 == ':') //使用作用域限定符"::"调用函数，例如 ret = A::set(123);
	{
		RETURN_IF_FAILED(p21 - 1 < p1, -1);
		p21--;

		RETURN_IF_FAILED(*p21 != ':', -1);

		classInstance.instanceType = VAR_TYPE_STATIC2;
	}

	//--------反向查找类的实例名--------------
	RETURN_IF_FAILED(p21 - 1 < p1, -1);
	p21--;

	ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
	RETURN_IF_FAILED(ret, ret);

	if (classInstance.instanceType == VAR_TYPE_NORMAL || classInstance.instanceType == VAR_TYPE_POINTER)
	{
		classInstance.classInstanceName.end = p21;
		classInstance.classInstanceName.fileOffsetOfEnd = p21 - p11;
		classInstance.classInstanceName.lineNumberOfEnd = lineNumber;

		if (*p21 == ']') //类似 int len = a[i][j].length();
		{
			while (p21 >= p1 && *p21 == ']')
			{
				ret = findPairCharBack(p1, p21 - p1 + 1, p21, '[', ']', p21);
				if (ret != 0)
				{
					break;
				}
				p21--;
			}
		}

		ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
		RETURN_IF_FAILED(ret, ret);

		classInstance.classInstanceName.start = p21;
		classInstance.classInstanceName.fileOffsetOfStart = p21 - p11;
		classInstance.classInstanceName.lineNumberOfStart = lineNumber;

		classInstance.classInstanceName.length = classInstance.classInstanceName.end - classInstance.classInstanceName.start + 1;
		classInstance.classInstanceName.copyStrFromBuffer();
	}
	else if (classInstance.instanceType == VAR_TYPE_STATIC2)
	{
		//--------反向查找类名(或命名空间名namespace)--------------
		classInstance.className.end = p21;
		classInstance.className.fileOffsetOfEnd = p21 - p11;
		classInstance.className.lineNumberOfEnd = lineNumber;

		ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
		RETURN_IF_FAILED(ret, ret);

		classInstance.className.start = p21;
		classInstance.className.fileOffsetOfStart = p21 - p11;
		classInstance.className.lineNumberOfStart = lineNumber;

		classInstance.className.length = classInstance.className.end - classInstance.className.start + 1;
		classInstance.className.copyStrFromBuffer();
	}

	//--------反向查找函数调用的返回值--------------
	RETURN_IF_FAILED(p21 - 1 < p1, -1);
	p21--;

	ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
	RETURN_IF_FAILED(ret, ret);

	if (*p21 == '=') //出现等号'='，说明有返回值
	{
		RETURN_IF_FAILED(p21 - 1 < p1, -1);
		p21--;

		if (*p21 == '=') //类似 if( true == A.get() ){}
		{
			return 0;
		}

		ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
		RETURN_IF_FAILED(ret, ret);

		//--------反向查找返回值变量名--------------
		classInstance.functionReturnValue.end = p21;
		classInstance.functionReturnValue.fileOffsetOfEnd = p21 - p11;
		classInstance.functionReturnValue.lineNumberOfEnd = lineNumber;

		ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
		RETURN_IF_FAILED(ret, ret);

		classInstance.functionReturnValue.start = p21;
		classInstance.functionReturnValue.fileOffsetOfStart = p21 - p11;
		classInstance.functionReturnValue.lineNumberOfStart = lineNumber;

		classInstance.functionReturnValue.length = classInstance.functionReturnValue.end - classInstance.functionReturnValue.start + 1;
		classInstance.functionReturnValue.copyStrFromBuffer();
		
		p21--;
	}
	
	//-------在函数体内部尝试反向查找实例对应的类名（如果是类的成员变量则会查不到）-------------
	if (classInstance.instanceType == VAR_TYPE_NORMAL || classInstance.instanceType == VAR_TYPE_POINTER)
	{
		std::string classInstanceName  = classInstance.classInstanceName.str;
		std::string varDeclareType = "";
		ret = findVarDeclareBack(p1, p21 - p1 + 1, classInstanceName, varDeclareType);
		if (ret == 0)
		{
			int len = MIN(varDeclareType.length(), sizeof(classInstance.className.str) - 1);
			memcpy(classInstance.className.str, varDeclareType.c_str(), len);
			classInstance.className.str[len] = '\0';
		}
	}
	
	//-------在函数参数列表中尝试反向查找实例对应的类名（如果是类的成员变量则会查不到）-------------
	if (classInstance.instanceType == VAR_TYPE_NORMAL || classInstance.instanceType == VAR_TYPE_POINTER)
	{
//		std::string classInstanceName  = classInstance.classInstanceName.str;
//		std::string varDeclareType = "";
	}

	return 0;
}


int CFuncRoute::findWholeFuncDeclare(unsigned char *buffer, int bufferSize, unsigned char *parentheseLeft, FUNCTION_STRUCTURE &funcDeclare, unsigned char *bufferBase, int lineNumberBase)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p11 = bufferBase;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int lineNumber = lineNumberBase;

	int flag = 0;

	p21 = parentheseLeft; //函数调用的参数列表左小括号位置

	//-------前向查找配对的右小括号------------
	funcDeclare.functionParameter.start = p21;
	funcDeclare.functionParameter.fileOffsetOfStart = p21 - p11;
	funcDeclare.functionParameter.lineNumberOfStart = lineNumber;

	ret = findPairCharForward(p21, p3 - p21 + 1, p21, '(', ')', p21);
	RETURN_IF_FAILED(ret, ret);
	
	funcDeclare.functionParameter.end = p21;
	funcDeclare.functionParameter.fileOffsetOfEnd = p21 - p11;
	funcDeclare.functionParameter.lineNumberOfEnd = lineNumber;

	funcDeclare.functionParameter.length = funcDeclare.functionParameter.end - funcDeclare.functionParameter.start + 1;
	funcDeclare.functionParameter.copyStrFromBuffer();

	//------查找左小括号前面的第一个非空白字符--------------
	p21 = parentheseLeft - 1;

	ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
	RETURN_IF_FAILED(ret, ret);

	//-------反向查找函数名------------
	funcDeclare.functionName.end = p21;
	funcDeclare.functionName.fileOffsetOfEnd = p21 - p11;
	funcDeclare.functionName.lineNumberOfEnd = lineNumber;

	if (*p21 == '>') //类似 std::TemplateA<std::string>(a, b); 模板函数调用
	{
		ret = findPairCharBack(p1, p21 - p1 + 1, p21, '<', '>', p21);
		RETURN_IF_FAILED(ret, ret);

		p21--;
		RETURN_IF_FAILED(p21 < p1, -1);
	}

	ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
	if (ret != 0) //类似 char * str = (char *)malloc(2);
	{
		return -1;
	}
	
	if (p21 - 1 >= p1 && *(p21 - 1) == '~') //析构函数，类似 class A {public: ~A();};
	{
		p21--;
	}

	funcDeclare.functionName.start = p21;
	funcDeclare.functionName.fileOffsetOfStart = p21 - p11;
	funcDeclare.functionName.lineNumberOfStart = lineNumber;

	funcDeclare.functionName.length = funcDeclare.functionName.end - funcDeclare.functionName.start + 1;
	funcDeclare.functionName.copyStrFromBuffer();

	if (isKeyword((unsigned char *)funcDeclare.functionName.str, funcDeclare.functionName.length) == true) //类似 if( a == 2 ) 被当成函数调用了
	{
		return -1;
	}

	//-------反向查找函数返回类型名------------
	p21--;
	ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
	RETURN_IF_FAILED(ret, ret);

	//---------------------
	funcDeclare.functionReturnValue.end = p21;
	funcDeclare.functionReturnValue.fileOffsetOfEnd = p21 - p11;
	funcDeclare.functionReturnValue.lineNumberOfEnd = lineNumber;

	if (*p21 == '>') //类似 std::vector<std::string> a;
	{
		ret = findPairCharBack(p1, p21 - p1 + 1, p21, '<', '>', p21);
		RETURN_IF_FAILED(ret, ret);
		p21--;
	}

	ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
	if (ret == 0)
	{
		if (p21 - 2 >= p1 && *(p21 - 1) == ':' && *(p21 - 2) == ':') //类似 std::string get(int a);
		{
			p21 -= 3;
			ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
			if (ret != 0)
			{
				return 0;
			}
		}

		funcDeclare.functionReturnValue.start = p21;
		funcDeclare.functionReturnValue.fileOffsetOfStart = p21 - p11;
		funcDeclare.functionReturnValue.lineNumberOfStart = lineNumber;

		funcDeclare.functionReturnValue.length = funcDeclare.functionReturnValue.end - funcDeclare.functionReturnValue.start + 1;
		funcDeclare.functionReturnValue.copyStrFromBuffer();

		ret = replaceTwoMoreWhiteSpaceByOneSpace((unsigned char *)funcDeclare.functionReturnValue.str, funcDeclare.functionReturnValue.length);

		return 0; //找到了
	}

	return 0;
}


int CFuncRoute::skipWhiteSpaceForward(unsigned char *buffer, int bufferSize, unsigned char *leftPos, unsigned char *&rightPos, int &lineNumber)
{
	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	int flag = 0;

	p21 = leftPos;

	while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
	{
		if (*p21 == '\n')
		{
			lineNumber++;
		}
		p21++;
	}

	if (p21 <= p3)
	{
		rightPos = p21;
		return 0;
	}

	return -1;
}


int CFuncRoute::skipWhiteSpaceBack(unsigned char *buffer, int bufferSize, unsigned char *rightPos, unsigned char *&leftPos, int &lineNumber)
{
	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	int flag = 0;

	p21 = rightPos;

	while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
	{
		if (*p21 == '\n')
		{
			lineNumber--;
		}
		p21--;
	}

	if (p21 >= p1)
	{
		leftPos = p21;
		return 0;
	}

	return -1;
}


int CFuncRoute::findPairCharForward(unsigned char *buffer, int bufferSize, unsigned char *leftCharPos, char leftChar, char rightChar, unsigned char *&rightCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	int flag = 0;

	p21 = leftCharPos; //函数调用的参数列表左小括号位置
	while (p21 <= p3)
	{
		if (*p21 == leftChar) //类似 int ret = A.set(B.get());
		{
			flag++;
		}
		else
		{
			if (*p21 == rightChar)
			{
				if (flag == 1)
				{
					rightCharPos = p21; //找到右小括号的位置
					return 0;
				}
				else
				{
					flag--;
				}
			}
		}
		p21++;
	}

	return -1;
}


int CFuncRoute::findPairCharBack(unsigned char *buffer, int bufferSize, unsigned char *rightCharPos, char leftChar, char rightChar, unsigned char *&leftCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	int flag = 0;

	p21 = rightCharPos; //函数调用的参数列表右小括号位置
	while (p21 >= p1)
	{
		if (*p21 == rightChar) //类似 int ret = A.set(B.get());
		{
			flag++;
		}
		else
		{
			if (*p21 == leftChar)
			{
				if (flag == 1)
				{
					leftCharPos = p21; //找到左小括号的位置
					return 0;
				}
				else
				{
					flag--;
				}
			}
		}
		p21--;
	}

	return -1;
}


int CFuncRoute::findPairCharBackStop(unsigned char *buffer, int bufferSize, unsigned char *rightCharPos, char leftChar, char rightChar, char *stopChar, unsigned char *&leftCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	int stopCharLen = strlen(stopChar);
	int flag = 0;

	p21 = rightCharPos; //函数调用的参数列表右小括号位置
	while (p21 >= p1)
	{
		for (int i = 0; i < stopCharLen; ++i)
		{
			if (*p21 == stopChar[i]) //遇到停止字符，则返回失败
			{
				return -1;
			}
		}

		if (*p21 == rightChar) //类似 int ret = A.set(B.get());
		{
			flag++;
		}
		else
		{
			if (*p21 == leftChar)
			{
				if (flag == 1)
				{
					leftCharPos = p21; //找到左小括号的位置
					return 0;
				}
				else
				{
					flag--;
				}
			}
		}
		p21--;
	}

	return -1;
}


int CFuncRoute::findCharForward(unsigned char *buffer, int bufferSize, char ch, unsigned char *&rightCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;

	p21 = p2;
	while (p21 <= p3)
	{
		if (*p21 == ch)
		{
			rightCharPos = p21;
			return 0;
		}

		p21++;
	}

	return -1;
}


int CFuncRoute::findCharForwardStop(unsigned char *buffer, int bufferSize, char ch, char *stopChar, unsigned char *&rightCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	int stopCharLen = strlen(stopChar);

	p21 = p2;
	while (p21 <= p3)
	{
		for (int i = 0; i < stopCharLen; ++i)
		{
			if (*p21 == stopChar[i]) //遇到停止字符，则返回失败
			{
				return -1;
			}
		}

		if (*p21 == ch)
		{
			rightCharPos = p21;
			return 0;
		}

		p21++;
	}

	return -1;
}


int CFuncRoute::findCharBack(unsigned char *buffer, int bufferSize, char ch, unsigned char *&leftCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;

	p21 = p3;
	while (p21 >= p1)
	{
		if (*p21 == ch)
		{
			leftCharPos = p21;
			return 0;
		}

		p21--;
	}

	return -1;
}


int CFuncRoute::findCharsBackGreedy(unsigned char *buffer, int bufferSize, char *chars, unsigned char *&leftCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	int charsLen = strlen(chars);

	p21 = p3;
	while (p21 >= p1)
	{
		for (int i = 0; i < charsLen; ++i)
		{
			if (*p21 == chars[i]) //遇到停止字符，则返回失败
			{
				leftCharPos = p21;
				return 0;
			}
		}

		p21--;
	}
	
	leftCharPos = p21;

	return -1;
}


int CFuncRoute::findCharBackStop(unsigned char *buffer, int bufferSize, char ch, char *stopChar, unsigned char *&leftCharPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	int stopCharLen = strlen(stopChar);

	p21 = p3;
	while (p21 >= p1)
	{
		for (int i = 0; i < stopCharLen; ++i)
		{
			if (*p21 == stopChar[i]) //遇到停止字符，则返回失败
			{
				return -1;
			}
		}

		if (*p21 == ch)
		{
			leftCharPos = p21;
			return 0;
		}

		p21--;
	}

	return -1;
}


int CFuncRoute::findStrForward(unsigned char *buffer, int bufferSize, unsigned char *leftPos, unsigned char *&rightPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int flag = 0;

	p21 = leftPos;
	while (p21 <= p3)
	{
		if ((*p21 >= '0' && *p21 <= '9')
			|| (*p21 >= 'a' && *p21 <= 'z')
			|| (*p21 >= 'A' && *p21 <= 'Z')
			|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
			)
		{
			flag = 1;
		}
		else
		{
			if (flag == 1)
			{
				rightPos = p21 - 1;
				return 0;
			}
		}

		p21++;
	}

	return -1;
}


int CFuncRoute::findStrBack(unsigned char *buffer, int bufferSize, unsigned char *rightPos, unsigned char *&leftPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int flag = 0;

	p21 = rightPos;
	while (p21 >= p1)
	{
		if ((*p21 >= '0' && *p21 <= '9')
			|| (*p21 >= 'a' && *p21 <= 'z')
			|| (*p21 >= 'A' && *p21 <= 'Z')
			|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
			)
		{
			flag = 1;
		}
		else
		{
			if (flag == 1)
			{
				leftPos = p21 + 1;
				return 0;
			}
			else
			{
				return -1;
			}
		}

		p21--;
	}

	return -1;
}


int CFuncRoute::findStrCharsBackGreedy(unsigned char *buffer, int bufferSize, unsigned char *rightPos, char *chars, unsigned char *&leftPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int charsLen = strlen(chars);

	p21 = rightPos;
	while (p21 >= p1)
	{
		if ((*p21 >= '0' && *p21 <= '9')
			|| (*p21 >= 'a' && *p21 <= 'z')
			|| (*p21 >= 'A' && *p21 <= 'Z')
			|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
			)
		{
			leftPos = p21;
			return 0;
		}
		
		for (int i = 0; i < charsLen; ++i)
		{
			if (*p21 == chars[i])
			{
				leftPos = p21;
				return 0;
			}
		}

		p21--;
	}

	return -1;
}


int CFuncRoute::findQueryStrBackStop(unsigned char *buffer, int bufferSize, unsigned char *rightPos, char *queryStr, char *stopChar, unsigned char *&leftPos)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int flag = 0;
	int queryStrLen = strlen(queryStr);
	int stopCharLen = strlen(stopChar);

	p21 = rightPos - queryStrLen + 1;
	while (p21 >= p1)
	{
		for (int i = 0; i < stopCharLen; ++i)
		{
			if (*p21 == stopChar[i]) //遇到停止字符，则返回失败
			{
				return -1;
			}
		}

		if (memcmp(p21, queryStr, queryStrLen) == 0)
		{
			flag = 1;
		}
		else
		{
			if (flag == 1)
			{
				leftPos = p21 + 1;
				return 0;
			}
		}

		p21--;
	}

	return -1;
}


int CFuncRoute::findVarDeclareForward(unsigned char *buffer, int bufferSize, std::string queryStr, std::string &varDeclareType)
{
	int ret = 0;

	return -1;
}


int CFuncRoute::findVarDeclareBack(unsigned char *buffer, int bufferSize, std::string queryStr, std::string &varDeclareType)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int lineNumber = 1;
	int flag = 0;

	p21 = p3 - queryStr.length();
	while (p21 >= p1)
	{
		if (memcmp(p21, queryStr.c_str(), queryStr.length()) == 0)
		{
			p22 = p21 + queryStr.length();
			if (p22 <= p3)
			{
				if (isValidVarChar(*p22) == false) //确保是 B var; 而不是 B varB; 类型
				{
					p22 = p21 - 1;
					if (p22 >= p1)
					{
						if (isValidVarChar(*p22) == false)//确保是 B var; 而不是 B Cvar; 类型
						{
							p21--;

							ret = skipWhiteSpaceBack(p1, p21 - p1, p21, p21, lineNumber);
							RETURN_IF_FAILED(ret, ret);

							//---------------------
							STRING_POSITON sp;
							memset(&sp, 0, sizeof(STRING_POSITON));

							sp.end = p21;

							if (*p21 == '>') //类似 std::vector<std::string> a;
							{
								ret = findPairCharBack(p1, p21 - p1 + 1, p21, '<', '>', p21);
								RETURN_IF_FAILED(ret, ret);
								p21--;
							}

							ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
							if (ret == 0)
							{
								if (p21 - 2 >= p1 && *(p21 - 1) == ':' && *(p21 - 2) == ':') //类似 std::string a;
								{
									p21 -= 3;
									ret = findStrBack(p1, p21 - p1 + 1, p21, p21);
									if (ret != 0)
									{
										continue;
									}
								}

								sp.start = p21;
								sp.length = sp.end - sp.start + 1;
								sp.copyStrFromBuffer();

								ret = replaceTwoMoreWhiteSpaceByOneSpace((unsigned char *)sp.str, sp.length);

								varDeclareType = sp.str;
								return 0; //找到了
							}
						}
					}
				}
			}
		}

		p21--;
	}

	return -1;
}


bool CFuncRoute::isValidVarChar(char ch)
{
	if ((ch >= '0' && ch <= '9')
		|| (ch >= 'a' && ch <= 'z')
		|| (ch >= 'A' && ch <= 'Z')
		|| (ch == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
		)
	{
		return true;
	}
	return false;
}


bool CFuncRoute::isWhiteSpace(char ch)
{
	if (ch == ' ' //空格字符(space character)
		|| ch == '\t' //制表符(tab character)
		|| ch == '\r' //回车符(carriage return)
		|| ch == '\n' //换行符(new line)
		)
	{
		return true;
	}
	return false;
}


int CFuncRoute::replaceTwoMoreWhiteSpaceByOneSpace(unsigned char *buffer, int bufferSize)
{
	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	int whiteSpaceFlag = 0;
	int pos = 0;
	p21 = p1;

	while (p21 <= p3)
	{
		if (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n') //空白字符
		{
			whiteSpaceFlag = 1;
		}
		else
		{
			if (pos < bufferSize)
			{
				if (whiteSpaceFlag == 1 && pos != 0)
				{
					whiteSpaceFlag = 0;
					p1[pos] = ' '; //多个连续空白字符，用一个空格代替
					pos++;
				}
				p1[pos] = *p21;
				pos++;
			}
		}

		p21++;
	}

	p1[pos] = '\0';

	return 0;
}


int CFuncRoute::statBufferLinesCount(unsigned char *buffer, int bufferSize)
{
	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;

	int lineCount = 0;
	p21 = p1;

	while (p21 <= p3)
	{
		if (*p21 == '\n') //换行符
		{
			lineCount++;
		}

		p21++;
	}

	return lineCount;
}


int CFuncRoute::macroExpand()
{
	return 0;
}


bool CFuncRoute::isParentClass(std::string child, std::string parent, std::vector<FUNCTIONS> &vFunctions)
{
	//-------判断parent类是否是child的父类-----------
	int len1 = vFunctions.size();

	for (int i = 0; i < len1; ++i)
	{
		int len2 = vFunctions[i].classes.size();
		for (int j = 0; j < len2; ++j)
		{
			std::string className = vFunctions[i].classes[j].className.str;
			if (className == child) //先找到child对应的位置
			{
				int len3 = vFunctions[i].classes[j].classParents.size();
				for (int k = 0; k < len3; ++k)
				{
					std::string classParent = vFunctions[i].classes[j].classParents[k].str;
					if (classParent == parent) //说明parent是child的父类
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}


int CFuncRoute::updateParentClass(std::vector<FUNCTIONS> &vFunctions)
{
	int ret = 0;

	//-------更新C++类的父类------------
	int len1 = vFunctions.size();

	for (int i = 0; i < len1; ++i)
	{
		int len2 = vFunctions[i].classes.size();
		for (int j = 0; j < len2; ++j)
		{
			int len3 = vFunctions[i].classes[j].classParents.size();

			for (int k = 0; k < len3; ++k)
			{
				std::string className1 = vFunctions[i].classes[j].classParents[k].str;

				//------------------------------------
				for (int i2 = 0; i2 < len1; ++i2)
				{
					int len22 = vFunctions[i2].classes.size();
					for (int j2 = 0; j2 < len22; ++j2)
					{
						std::string className2 = vFunctions[i2].classes[j2].className.str;

						if (i != i2 && j != j2 && className1 == className2) //确保是同一个C++类名
						{
							int len4 = vFunctions[i2].classes[j2].classParents.size();
							for (int k2 = 0; k2 < len4; ++k2)
							{
								std::string className21 = vFunctions[i2].classes[j2].classParents[k2].str;

								int flag = 0;
								int len32 = vFunctions[i].classes[j].classParents.size();
								for (int k3 = 0; k3 < len32; ++k3)
								{
									std::string className11 = vFunctions[i].classes[j].classParents[k3].str;
									if (className21 == className11) //出现 "class A1 ： public B1, public C1{};" 和 "class A2 ： public B2, public C1{};" 共同的父类 "public C1"，则跳过
									{
										flag = 1;
										break;
									}
								}

								if (flag == 0)
								{
									vFunctions[i].classes[j].classParents.push_back(vFunctions[i2].classes[j2].classParents[k2]);
								}
							}
						}
					}
				}
			}
		}
	}

	return ret;
}


int CFuncRoute::splitParentsClass(unsigned char *buffer, int bufferSize, std::vector<MY_STRING> &classParents)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;

	char publicClass[] = "public";
	char protectedClass[] = "protected";
	char privateClass[] = "private";

	while (p2 <= p3)
	{
		p1 = p2;
		while (p2 < p3 && *p2 != ',') //类之间用逗号隔开的，"class B : public A, public C{};"
		{
			*p2++;
		}

		if (p2 <= p3)
		{
			if(*p2 == ',')
			{
				p2--;
			}

			p21 = p1;

			while (p21 <= p2 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
			{
				p21++;
			}

			if (p21 <= p2)
			{
				//------跳过修饰C++类的关键字"public/protected/private"--------
				if (memcmp(p21, publicClass, strlen(publicClass)) == 0)
				{
					p21 += strlen(publicClass);
				}
				else if (memcmp(p21, protectedClass, strlen(protectedClass)) == 0)
				{
					p21 += strlen(protectedClass);
				}
				else if (memcmp(p21, privateClass, strlen(privateClass)) == 0)
				{
					p21 += strlen(privateClass);
				}
				else
				{
					ret = -1;
					printf("%s(%d): not in [public, protected, private]; ret=%d;\n", __FUNCTION__, __LINE__, ret);
					break;
				}

				if (p21 <= p2)
				{
					while (p21 <= p2 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
					{
						p21++;
					}

					//----------查找父类名-------------
					p22 = p21;

					while (p21 < p2)
					{
						if (!((*p21 >= '0' && *p21 <= '9')
							|| (*p21 >= 'a' && *p21 <= 'z')
							|| (*p21 >= 'A' && *p21 <= 'Z')
							|| (*p21 == '_')
							)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
						{
							break;
						}
						p21++;
					}

					if (p21 <= p2 && p21 >= p22)
					{
						MY_STRING myStrParentClass;
						memset(&myStrParentClass, 0, sizeof(MY_STRING));

						int len = MIN(p21 - p22 + 1, sizeof(myStrParentClass.str) - 1);
						memcpy(myStrParentClass.str, p22, len);
						myStrParentClass.str[len] = '\0';

						classParents.push_back(myStrParentClass);
					}
				}
			}
		}

		p2++;
		if(*p2 == ',')
		{
			p2++;
		}
	}

	return ret;
}

/*
int CFuncRoute::findAllMemberVarsInClassDeclare(unsigned char *buffer, int bufferSize, CLASS_STRUCT &classes, unsigned char *bufferBase, int lineNumberBase)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p11 = bufferBase;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	unsigned char *p23 = NULL;
	int lineNumber = lineNumberBase;
	int lineNumberTemp = lineNumber;
	bool ret2 = false;

	if (*p2 == '{') //跳过第一个字符是左大括号的情况
	{
		p2++;
	}

	//--------在类/结构体的声明语句块内部，提取出所有声明的成员变量---------------
	while (p2 <= p3)
	{
		//--------跳过函数块-----------------
		int curlyBracesFlag = 0; //花括号

		if (*p2 == '{')
		{
			while (p2 <= p3)
			{
				if (*p2 == '{')
				{
					curlyBracesFlag++;
				}else if (*p2 == '}')
				{
					if (curlyBracesFlag == 0)
					{
						break;
					}
					else
					{
						curlyBracesFlag--;
					}
				}
				else if (*p2 == '\n')
				{
					lineNumber++;
				}
				p2++;
			}

			p2++;
			if (p2 >= p3)
			{
				break;
			}

			p1 = p2; //更新 p1 的值
		}

		//---------------------
		if (*p2 == ';') //每个变量的声明都必须以分号';'结束
		{
			//-----向前搜索整条语句----
			int equalSignFlag = 0; //等号
			int parenthesesLeftFlag = 0; //左小括号
			int parenthesesRightFlag = 0; //右小括号

			p21 = p2 - 1;
			p22 = p21;

			while (p21 >= p1)
			{
				if (((*p21 == ';') //上一条语句结尾
					|| (*p21 == '{') //语句块开始
					|| (*p21 == '}') //语句块结束
					|| (*p21 == ':') //类似 "public: A a;"
					//|| (*p21 == ')') //类似 if(b) A a; 但这种写法，是没有任何意义的
					)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
				{
					break;
				}
				else if (*p21 == '=') //类似 virtual int func1() = 0; 或者 int operator=(int &i); 或者比较奇葩的写法 class A {int m_a = 0;}; 即在类中声明时，初始化成员变量
				{
					equalSignFlag++;
				}
				else if (*p21 == ')') //类似 int func2();
				{
					parenthesesLeftFlag++;
				}
				else if (*p21 == ')') //类似 int func2();
				{
					parenthesesRightFlag++;
				}
				else if (*p21 == '\n')
				{
					lineNumber--;
				}

				p21--;
			}

			//--------判断是函数声明还是变量声明------------
			if (p21 >= p1 && p21 < p22)
			{
				if (parenthesesLeftFlag > 0 && parenthesesRightFlag > 0 && parenthesesLeftFlag == parenthesesRightFlag) //说明是函数声明
				{
					//do nothing
				}
				else //说明是变量声明，类似：unsgned int m_a;
				{
					if (equalSignFlag == 0)
					{
						VAR_DECLARE var;
						memset(&var, 0, sizeof(VAR_DECLARE));

						p23 = p21 + 1;
						p21 = p22;
						lineNumberTemp = lineNumber;

						while (p21 >= p23 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
						{
							if (*p21 == '\n')
							{
								lineNumber--;
							}
							p21--;
						}

						if (p21 >= p23)
						{
							//----查找变量名----
							var.varName.end = p21;
							var.varName.fileOffsetOfEnd = var.varName.end - p11;
							var.varName.lineNumberOfEnd = lineNumber;

							p22 = p21;

							while (p21 >= p23)
							{
								if (!((*p21 >= '0' && *p21 <= '9')
									|| (*p21 >= 'a' && *p21 <= 'z')
									|| (*p21 >= 'A' && *p21 <= 'Z')
									|| (*p21 == '_')
									)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
								{
									break;
								}
								p21--;
							}

							if (p21 >= p23 && p21 < p22)
							{
								var.varName.start = p21 + 1;
								var.varName.fileOffsetOfStart = var.varName.start - p11;
								var.varName.lineNumberOfStart = lineNumber;
								var.varName.length = var.varName.end - var.varName.start + 1;

								var.varName.copyStrFromBuffer();

								//-------继续查找变量类型名----------
								while (p21 >= p23 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
								{
									if (*p21 == '\n')
									{
										lineNumber--;
									}
									p21--;
								}

								if (p21 >= p23)
								{
									var.varType.end = p21;
									var.varType.fileOffsetOfEnd = var.varType.end - p11;
									var.varType.lineNumberOfEnd = lineNumber;

									p21 = p23;
									lineNumber = lineNumberTemp;

									while (p21 >= p23 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
									{
										if (*p21 == '\n')
										{
											lineNumber++;
										}
										p21++;
									}

									if (p21 <= var.varType.end)
									{
										var.varType.start = p21;
										var.varType.fileOffsetOfStart = var.varType.start - p11;
										var.varType.lineNumberOfStart = lineNumber;
										var.varType.length = var.varType.end - var.varType.start + 1;

										var.varType.copyStrFromBuffer();

										classes.memberVars.push_back(var);

										p1 = p2; //更新 p1 的值
									}
								}
							}
						}
					}
					else
					{
						printf("%s(%d): WARN: not suppport like 'class A {int m_a = 0;};'\n", __FUNCTION__, __LINE__);
					}
				}
			}
		}
		
		if (*p2 == '\n')
		{
			lineNumber++;
		}

		p2++;
	}

	return ret;
}
*/

int CFuncRoute::findAllMemberVarsInClassDeclare(unsigned char *buffer, int bufferSize, CLASS_STRUCT &classes, unsigned char *bufferBase, int lineNumberBase)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p11 = bufferBase;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	unsigned char *p23 = NULL;
	int lineNumber = lineNumberBase;
	int lineNumberTemp = lineNumber;
	bool ret2 = false;
	unsigned char * pSemicolon = 0; //分号

	p21 = p2;
	if (*p21 == '{') //跳过第一个字符是左大括号的情况
	{
		p21++;
	}

	//--------在类/结构体的声明语句块内部，提取出所有声明的成员变量---------------
	while (p21 <= p3)
	{
		//--------查找标志语句结束的分号';'--------
		ret = findCharForward(p21, p3 - p21 + 1, ';', p22);
		if(ret != 0)
		{
			break;
		}

		pSemicolon = p22;

		ret = findCharBackStop(p21, p22 - p21 + 1, '{', "}", p23); //说明';'所在的语句处于某个语句块内部，需要跳过
		if(ret == 0)
		{
			ret = findPairCharForward(p23, p3 - p23 + 1, p23, '{', '}', p21);
			RETURN_IF_FAILED(ret, ret);
			p21++;
			continue;
		}
		
		ret = findCharBackStop(p21, p22 - p21 + 1, '(', ")", p23); //类似: "for(int i = 0; i < 2; ++i)"，需要跳过
		if(ret == 0)
		{
			ret = findPairCharForward(p23, p3 - p23 + 1, p23, '(', ')', p21);
			RETURN_IF_FAILED(ret, ret);
			p21++;
			continue;
		}

		//----查找变量名----
		VAR_DECLARE var;
		memset(&var, 0, sizeof(VAR_DECLARE));
		
		p22--;
		
		ret = skipWhiteSpaceBack(p21, p22 - p21 + 1, p22, p22, lineNumber);
		if(ret != 0)
		{
			p21 = pSemicolon + 1;
			continue;
		}

		var.varName.end = p22;
		var.varName.fileOffsetOfEnd = var.varName.end - p11;
		lineNumber = lineNumberBase + statBufferLinesCount(p2, p22 - p2 + 1);
		var.varName.lineNumberOfEnd = lineNumber;

		if(*var.varName.end == ']')
		{
			ret = findPairCharBack(p21, var.varName.end - p21 + 1, p22, '[', ']', p22); //类似: int a[20];
			if(ret == 0)
			{
				p22--;
			}
		}

		ret = findStrBack(p21, p22 - p21 + 1, p22, p22);
		if(ret != 0)
		{
			p21 = pSemicolon + 1;
			continue;
		}
		
		var.varName.start = p22;
		var.varName.fileOffsetOfStart = var.varName.start - p11;
		var.varName.lineNumberOfStart = lineNumber;
		
		var.varName.length = var.varName.end - var.varName.start + 1;
		var.varName.copyStrFromBuffer();
		
		//----查找变量类型名----
		p22--;
		ret = skipWhiteSpaceBack(p21, p22 - p21 + 1, p22, p22, lineNumber);

		ret = findStrCharsBackGreedy(p21, p22 - p21 + 1, p22, ">", p22); //类似: std::vector<int> a;
		RETURN_IF_FAILED(ret, ret);
		
		var.varType.end = p22;
		var.varType.fileOffsetOfEnd = var.varType.end - p11;
		lineNumber = lineNumberBase + statBufferLinesCount(p2, p22 - p2 + 1);
		var.varType.lineNumberOfEnd = lineNumber;
		
		ret = findCharsBackGreedy(p21, p22 - p21 + 1, ";:{}", p23);
		if(ret != 0 && p23 < p21)
		{
			p23 = p21;
		}
		
		p23++;
		ret = skipWhiteSpaceForward(p23, p22 - p23 + 1, p23, p23, lineNumber);
		RETURN_IF_FAILED(ret, ret);
		
		var.varType.start = p23;
		var.varType.fileOffsetOfStart = var.varType.start - p11;
		lineNumber = lineNumberBase + statBufferLinesCount(p2, p23 - p2 + 1);
		var.varType.lineNumberOfStart = lineNumber;
		
		var.varType.length = var.varType.end - var.varType.start + 1;
		var.varType.copyStrFromBuffer();

		ret = replaceTwoMoreWhiteSpaceByOneSpace((unsigned char *)var.varType.str, var.varType.length);
		
		//----查找变量类型名和变量名之间是否有等号'='----
		ret = findCharForward(var.varType.start, var.varName.end - var.varType.start + 1, '=', p22); //类似: A *a = new A; 则需要跳过
		if(ret == 0)
		{
			p21 = pSemicolon + 1;
			continue;
		}

		//----查找变量类型名和变量名之间是否有星号'*'或取地址符'&'----
		var.isPointerVar = false;

		ret = findCharForward(var.varType.end, var.varName.start - var.varType.end + 1, '*', p22);
		if(ret == 0)
		{
			var.isPointerVar = true;
		}

		ret = findCharForward(var.varType.end, var.varName.start - var.varType.end + 1, '&', p22);
		if(ret == 0)
		{
			var.isPointerVar = true;
		}
		
		//--------查找到一个完整的变量声明-----------
		classes.memberVars.push_back(var);

		//-------------------
		p21 = pSemicolon + 1;
	}

	return ret;
}


int CFuncRoute::findAllMemberFuncsInClassDeclare(unsigned char *buffer, int bufferSize, CLASS_STRUCT &classes, unsigned char *bufferBase, int lineNumberBase)
{
	int ret = 0;

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize - 1;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int lineNumber = lineNumberBase;

	p21 = p1;
	while (p21 <= p3)
	{
		if (*p21 == '(')
		{
			FUNCTION_STRUCTURE fs;
			memset(&fs, 0, sizeof(FUNCTION_STRUCTURE));

			ret = findWholeFuncDeclare(p1, p3 - p1 + 1, p21, fs, bufferBase, lineNumberBase);
			if(ret == 0)
			{
				int len = MIN(strlen(classes.className.str), sizeof(fs.className) - 1);
				if(len > 0)
				{
					memcpy(fs.className, classes.className.str, len);
					fs.className[len] = '\0';
				}
				
				len = MIN(strlen(classes.classNameAlias.str), sizeof(fs.classNameAlias) - 1);
				if(len > 0)
				{
					memcpy(fs.classNameAlias, classes.classNameAlias.str, len);
					fs.classNameAlias[len] = '\0';
				}

				classes.memberFuncs.push_back(fs);
			}
		}else if (*p21 == '{' && p21 != p1) //FIXME: 跳过函数定义块，但是也会跳过类中类
		{
			ret = findPairCharForward(p21, p3 - p21, p21, '{', '}', p21);
		}

		p21++;
	}

	return 0;
}


bool CFuncRoute::isFunctionArgsMatch(std::string parameter, std::string functionArgs)
{
	const char * str1 = parameter.c_str();
	const char * str2 = functionArgs.c_str();

	int commaCnt1 = 0; //逗号
	int commaCnt2 = 0; //逗号

	for (int i = 0; i < strlen(str1); ++i)
	{
		if (str1[i] == ',')
		{
			commaCnt1++;
		}
	}

	for (int i = 0; i < strlen(str2); ++i)
	{
		if (str2[i] == ',')
		{
			commaCnt2++;
		}
	}

	if (commaCnt1 == commaCnt2) //FIXME: 目前只简单的比较函数参数列表的逗号个数是否相等
	{
		return true;
	}

	return false;
}


int CFuncRoute::statAllFuns(std::vector<FUNCTIONS> &vFunctions)
{
	int ret = 0;

	int funcCnt = 1;

	//-------先对所有函数进行编号------------
	int len1 = vFunctions.size();
	for (int i = 0; i < len1; ++i)
	{
		int len2 = vFunctions[i].funcs.size();
		for (int j = 0; j < len2; ++j)
		{
			vFunctions[i].funcs[j].functionIndex = funcCnt++;
		}
	}

	//-------更新父类的父类------------
	ret = updateParentClass(vFunctions);

	//-------更新函数体中C++类变量的类型------------
	for (int i = 0; i < len1; ++i)
	{
		int len2 = vFunctions[i].funcs.size();
		for (int j = 0; j < len2; ++j)
		{
			int len3 = vFunctions[i].funcs[j].funcsWhichInFunctionBody.size();
			for (int k = 0; k < len3; ++k)
			{
				int flag = 0;
				std::string classInstanceName1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].classInstanceName.str;
				std::string className1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str;
				std::string className12 = vFunctions[i].funcs[j].className;
				std::string classNameAlias12 = vFunctions[i].funcs[j].classNameAlias;
				std::string functionName1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionName.str;
				
				if (className1 == "") //实例没有填充类名的情况下，才遍历查找
				{
					for (int i2 = 0; i2 < len1; ++i2)
					{
						int len4 = vFunctions[i2].classes.size();
						for (int j2 = 0; j2 < len4; ++j2)
						{
							//---先在成员变量中查找---
							int len4 = vFunctions[i2].classes[j2].memberVars.size();
							for (int m2 = 0; m2 < len4; ++m2)
							{
								std::string varName = vFunctions[i2].classes[j2].memberVars[m2].varName.str;
								std::string varType = vFunctions[i2].classes[j2].memberVars[m2].varType.str;
								std::string className41 = vFunctions[i2].classes[j2].className.str;

								if (varName == classInstanceName1) //C++类的成员变量和成员函数体中的某个变量相等了
								{
									//--------尝试查找变量是否在本类或者父类中声明的，如果不是，则可能是全局变量----------------
									if (className41 == className12)
									{
										flag = 1;
									}
									else //再判断className12的父类中是否包含className41类
									{
										bool isParent = isParentClass(className12, className41, vFunctions);
										if (isParent == true)
										{
											flag = 1;
										}
									}

									//------------------------
									if (flag == 1)
									{
										int len63 = strlen(vFunctions[i2].classes[j2].memberVars[m2].varType.str);
										int len64 = MIN(len63, sizeof(vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str) - 1);

										memcpy(vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str, vFunctions[i2].classes[j2].memberVars[m2].varType.str, len64);
										vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str[len64] = '\0';
										break;
									}
								}
							}
							
							if (flag == 1)
							{
								break; //已经找到了，就不再继续找了
							}

							//---再在成员函数中查找---
							int len5 = vFunctions[i2].classes[j2].memberFuncs.size();
							for (int m2 = 0; m2 < len5; ++m2)
							{
								std::string functionName51 = vFunctions[i2].classes[j2].memberFuncs[m2].functionName.str;
								std::string className52 = vFunctions[i2].classes[j2].memberFuncs[m2].className;
								std::string classNameAlias53 = vFunctions[i2].classes[j2].memberFuncs[m2].classNameAlias;

								if (functionName51 == functionName1) //C++类的成员函数和某个函数体中的某个函数名相等了
								{
									int len63 = strlen(vFunctions[i2].classes[j2].memberFuncs[m2].className);
									int len64 = MIN(len63, sizeof(vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str) - 1);

									memcpy(vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str, vFunctions[i2].classes[j2].memberFuncs[m2].className, len64);
									vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str[len64] = '\0';
									flag = 1;
									break;
								}
							}
						}
						
						if (flag == 1)
						{
							break; //已经找到了，就不再继续找了
						}
					}
				}
			}
		}
	}

	//-------再查找某个函数被哪些函数调用了------------
	for (int i = 0; i < len1; ++i)
	{
		int len2 = vFunctions[i].funcs.size();

		for (int j = 0; j < len2; ++j)
		{
			int len3 = vFunctions[i].funcs[j].funcsWhichInFunctionBody.size();

			for (int k = 0; k < len3; ++k)
			{
				std::string functionName1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionName.str;
				std::string className1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str;
				std::string functionArgs1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionArgs.str;

				//-------在其他函数中遍历查找----------------
				for (int i2 = 0; i2 < len1; ++i2)
				{
					int len22 = vFunctions[i2].funcs.size();
					for (int j2 = 0; j2 < len22; ++j2)
					{
						std::string functionName2 = vFunctions[i2].funcs[j2].functionName.str;
						std::string className2 = vFunctions[i2].funcs[j2].className;
						std::string classNameAlias21 = vFunctions[i2].funcs[j2].classNameAlias;
						std::string functionParameter2 = vFunctions[i2].funcs[j2].functionParameter.str;

						size_t pos = functionName2.rfind("::");
						if (pos != std::string::npos)
						{
							functionName2 = functionName2.substr(pos + 2);
						}

						if (functionName2 == functionName1
							&& isFunctionArgsMatch(functionParameter2, functionArgs1)
							) //说明被这个函数调用了
						{
							bool isParent = isParentClass(className1, className2, vFunctions);

							if ((className1 != "" && className2 != "" && (className1 == className2) || isParent == true)
								|| (className1 != "" && classNameAlias21 != "" && className1 == classNameAlias21)
								|| (className1 == "" && className2 == "" && classNameAlias21 == "")
								)
							{
								if (vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionIndex == 0
									|| className1 == className2
									) //FIXME
								{
									vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionIndex = vFunctions[i2].funcs[j2].functionIndex;
								}
							}
//							vFunctions[i].funcs[j].funcsWhichCalledMe[vFunctions[i2].funcs[j2].functionIndex] += 1;
						}
					}
				}
			}
		}
	}

	//--------打印所有信息-------------------
//	ret = printInfo(vFunctions);

	//--------打印统计信息-------------------
	printf("============Total files：%d;=======\n", vFunctions.size());
	
	for (int i = 0; i < len1; ++i)
	{
		int len2 = vFunctions[i].funcs.size();

		for (int j = 0; j < len2; ++j)
		{
			std::string className = "";
			std::string str1 = vFunctions[i].funcs[j].classNameAlias + std::string("::");
			std::string str2 = vFunctions[i].funcs[j].functionName.str;
			
			if (strlen(vFunctions[i].funcs[j].className) > 0)
			{
				str1 = vFunctions[i].funcs[j].className + std::string("::");
			}

			if (str1 != "::" 
				&& !(str2.length() > str1.length() && str2.substr(0, str1.length()) == str1)
				)
			{
				className = str1;
			}

			printf("%s\t%d\t%d\t%s%s%s\t", vFunctions[i].fllename, vFunctions[i].funcs[j].functionName.lineNumberOfStart, vFunctions[i].funcs[j].functionIndex, 
				className.c_str(), vFunctions[i].funcs[j].functionName.str, vFunctions[i].funcs[j].functionParameter.str);
			
			int len3 = vFunctions[i].funcs[j].funcsWhichInFunctionBody.size();
			for (int k = 0; k < len3; ++k)
			{
				if (k != len3 - 1)
				{
					printf("%d,", vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionIndex);
				}
				else
				{
					printf("%d", vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].functionIndex);
				}
			}
/*
			printf("\t");

			int len4 = vFunctions[i].funcs[j].funcsWhichCalledMe.size();
			int cnt = 0;
			std::map<int, int>::iterator it;
			for (it = vFunctions[i].funcs[j].funcsWhichCalledMe.begin(); it != vFunctions[i].funcs[j].funcsWhichCalledMe.end(); ++it)
			{
				if (cnt <= len4 - 1)
				{
					printf("%d-%d,", it->first, it->second);
				}
				else
				{
					printf("%d-%d", it->first, it->second);
				}
				cnt++;
			}*/
			printf("\n");
		}
	}

	return ret;
}


int CFuncRoute::dumpBufferToFile(unsigned char *buffer, int bufferSize, char *filename)
{
	int ret = 0;

	RETURN_IF_FAILED(buffer == NULL || bufferSize <= 0 || filename == NULL, -1);

	printf("%s: filename=%s;\n", __FUNCTION__, filename);

	//-----------读取整个文件到内存中---------------
	FILE * fp = fopen(filename, "wb");
	RETURN_IF_FAILED(fp == NULL, -2);

	size_t read_size = fwrite(buffer, bufferSize, 1, fp);
	if (read_size != 1)
	{
		printf("%s: Error: read_size=%d != 1\n", __FUNCTION__, read_size);
		fclose(fp);
		return -6;
	}

	fclose(fp);

	return ret;
}


int CFuncRoute::printInfo(std::vector<FUNCTIONS> &vFunctions)
{
	int ret = 0;

	int len1 = vFunctions.size();
	for (int i = 0; i < len1; ++i)
	{
		printf("i=%d: ----%s-----\n", i, vFunctions[i].fllename);

		ret = vFunctions[i].printfInfo();
	}

	return ret;
}
