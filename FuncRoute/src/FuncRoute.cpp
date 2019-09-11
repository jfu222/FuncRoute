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
		
		ret = search_CPP_FuncName(buffer, file_size, functions);
		free(buffer);

		int len2 = functions.funcs.size();
		if (ret != 0 && len2 <= 0)
		{
			printf("[%d] %s; Warn: can not find any functions;\n", i, functions.fllename);
			continue;
		}

		int len = MIN(files2[i].length(), sizeof(functions.fllename) - 1);

		memcpy(functions.fllename, files2[i].c_str(), len);
		functions.fllename[len] = '\0';

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
	unsigned int lineNumber = 1;
	unsigned int lineNumberTemp = 1;
	unsigned char *p11 = NULL;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int braceCount = 0; //大括号对计数
	int parenthesesCount = 0; //小括号对计数
	int lenFuncString = sizeof(funcStruct.funcString);
	bool ret2 = false;

	int wordCountMax = 5; //函数名前面，最多搜索5个以空格隔开的字符串
	int wordCount = 0;
	int overSerach = 0; //停止搜索
	int lineCntMax = 5; //函数名前面，最多搜索5行
	int lineCnt = 0;

	//------先将被注释掉的代码用空格' '代替（行号符'\n'保留）--------
	ret = replaceAllCodeCommentsBySpace(buffer2, bufferSize);
	RETURN_IF_FAILED(ret, -2);

	//------查找所有的C++类名(关键字class, struct)--------
	char classKeyword[] = "class";
	char structKeyword[] = "struct";
	int classLen = strlen(classKeyword);
	int structLen = strlen(structKeyword);
	bool isStruct = false;

	//--------查找关键字class/struct----------------
	while (p2 <= p3 - structLen)
	{
		isStruct = false;

		if (memcmp(p2, classKeyword, classLen) == 0 || memcmp(p2, structKeyword, structLen) == 0)
		{
			int lenKeyword = classLen;
			if (memcmp(p2, structKeyword, structLen) == 0)
			{
				lenKeyword = structLen;
				isStruct = true;
			}

			if (p2 - 1 >= p1)
			{
				p21 = p2 - 1;
				if (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n' || *p21 == ';' || *p21 == '}') //关键字前面必须有一个空白字符
				{
					p21 = p2 + lenKeyword;
					while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过关键字class/struct后面空白字符
					{
						if (*p21 == '\n')
						{
							lineNumber++;
						}
						p21++;
					}

					if (p21 >= p2 + lenKeyword + 1) //关键字class/struct后面至少有一个空白字符
					{
						//-----查找C++类名-----
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

						if (p21 <= p3 && p21 > p22) //找到关键字class/struct了
						{
							CLASS_STRUCT cs;
							memset(&cs, 0, sizeof(CLASS_STRUCT));
							cs.isStruct = isStruct;

							cs.className.start = p22;
							cs.className.end = p21 - 1;
							cs.className.fileOffsetOfStart = cs.className.start - p1;
							cs.className.fileOffsetOfEnd = cs.className.end - p1;
							cs.className.lineNumberOfStart = lineNumber;
							cs.className.lineNumberOfEnd = lineNumber;
							cs.className.length = cs.className.end - cs.className.start + 1;

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
								if (*p21 == ':') //说明继承自父类，类似："class B : public A, public C{};"
								{
									p22 = p21;
									lineNumberTemp = lineNumber;
									while (p21 <= p3 && *p21 != '{')
									{
										if (*p21 == '\n')
										{
											lineNumber++;
										}
										p21++;
									}
									if (p21 < p3 && *p21 == '{')
									{
										cs.classParent.start = p22 + 1;
										cs.classParent.end = p21 - 1;
										cs.classParent.fileOffsetOfStart = cs.classParent.start - p1;
										cs.classParent.fileOffsetOfEnd = cs.classParent.end - p1;
										cs.classParent.lineNumberOfStart = lineNumberTemp;
										cs.classParent.lineNumberOfEnd = lineNumber;
										cs.classParent.length = cs.classParent.end - cs.classParent.start + 1;
										cs.classParent.copyStrFromBuffer();

										//-------拆分父类："class B : public A, public C{};"------------
										ret = splitParentsClass(cs.classParent.start, cs.classParent.length, cs.classParents);
									}
								}

								if (*p21 == '{') //说明没有父类，类似 public A{};
								{
									cs.className.copyStrFromBuffer();
									
									cs.classBody.start = p21;
									cs.classBody.fileOffsetOfStart = cs.classBody.start - p1;
									cs.classBody.lineNumberOfStart = lineNumber;
								}

								//-------查找C++类的体右大括号-----------
								p22 = p21;
								lineNumberTemp = lineNumber;
								int curlyBracketsCnt = 0; //大括号，需要跳过类的体内部的"{}"大括号对
								p21++;
								while (p21 <= p3)
								{
									if(*p21 == '{')
									{
										curlyBracketsCnt++;
									}else if(*p21 == '}')
									{
										if(curlyBracketsCnt == 0)
										{
											break;
										}
										curlyBracketsCnt--;
									}

									if (*p21 == '\n')
									{
										lineNumber++;
									}
									p21++;
								}

								if (*p21 == '}')
								{
									p21++;
									while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
									{
										if (*p21 == '\n')
										{
											lineNumber++;
										}
										p21++;
									}

									if (isStruct) //结构体的别名，类似 typedef struct _A_ {} A;
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

										if (p21 <= p3 && p21 > p22)
										{
											cs.classNameAlias.start = p22;
											cs.classNameAlias.end = p21 - 1;
											cs.classNameAlias.fileOffsetOfStart = cs.classNameAlias.start - p1;
											cs.classNameAlias.fileOffsetOfEnd = cs.classNameAlias.end - p1;
											cs.classNameAlias.lineNumberOfStart = lineNumber;
											cs.classNameAlias.lineNumberOfEnd = lineNumber;
											cs.classNameAlias.length = cs.classNameAlias.end - cs.classNameAlias.start + 1;
											cs.classNameAlias.copyStrFromBuffer();

											while (p21 <= p3 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
											{
												if (*p21 == '\n')
												{
													lineNumber++;
												}
												p21++;
											}
										}
									}

									if (p21 + 1 <= p3 && *p21 == ';') //说明找到了一个完整的C++类
									{
										cs.classBody.end = p21;
										cs.classBody.fileOffsetOfEnd = cs.classBody.end - p1;
										cs.classBody.lineNumberOfEnd = lineNumber;
										cs.classBody.length = cs.classBody.end - cs.classBody.start + 1;
										cs.classBody.copyStrFromBuffer();

										//-------在类/结构体的声明语句块内部，提取出所有声明的成员变量---------
										ret = findAllMemberVarsInClassDeclare(cs.classBody.start, cs.classBody.length, cs, p1, cs.classBody.lineNumberOfStart);

										functions.classes.push_back(cs);

										p2 = p21;
									}
								}
							}
						}
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
	
	//---------------------------
	p2 = buffer2;
	p21 = NULL;
	p22 = NULL;
	lineNumber = 1;

retry:
	while (p2 <= p3)
	{
		memset(&funcStruct, 0, sizeof(FUNCTION_STRUCTURE));

		//--------查找函数体左大括号----------------
		while (p2 <= p3 && *p2 != '{')
		{
			if (*p2 == '\n')
			{
				lineNumber++;
			}
			p2++;
		}

		if (p2 >= p3)
		{
			ret = -2;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		if (*p2 == '{')
		{
			funcStruct.functionBody.start = p2;
			funcStruct.functionBody.fileOffsetOfStart = funcStruct.functionBody.start - p1;
			funcStruct.functionBody.lineNumberOfStart = lineNumber;
		}

		//--------查找函数参数右小括号----------------
		lineNumberTemp = lineNumber;
		p21 = funcStruct.functionBody.start - 1;

		while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
		{
			if (*p21 == '\n')
			{
				lineNumber--;
			}
			p21--;
		}

		if (p21 <= p1)
		{
			ret = -3;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		if (*p21 == ')')
		{
			funcStruct.functionParameter.end = p21;
			funcStruct.functionParameter.fileOffsetOfEnd = funcStruct.functionParameter.end - p1;
			funcStruct.functionParameter.lineNumberOfEnd = lineNumber;
		}
		else
		{
			//-----尝试查找函数参数列表右小括号后面的C/C++修饰符(type-qualifier类型限定符)--------
			while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
			{
				if (*p21 == '\n')
				{
					lineNumber--;
				}
				p21--;
			}

			if (p21 <= p1)
			{
				ret = -6;
				printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
				break;
			}

			funcStruct.functionTypeQualifier.end = p21;
			funcStruct.functionTypeQualifier.fileOffsetOfEnd = funcStruct.functionTypeQualifier.end - p1;
			funcStruct.functionTypeQualifier.lineNumberOfEnd = lineNumber;

			while (p21 >= p1)
			{
				if ((*p21 >= '0' && *p21 <= '9')
					|| (*p21 >= 'a' && *p21 <= 'z')
					|| (*p21 >= 'A' && *p21 <= 'Z')
					|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
					)
				{

				}
				else
				{
					break;
				}

				p21--;
			}

			if (p21 <= p1)
			{
				ret = -7;
				printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
				break;
			}

			funcStruct.functionTypeQualifier.start = p21 + 1;
			funcStruct.functionTypeQualifier.fileOffsetOfStart = funcStruct.functionTypeQualifier.start - p1;
			funcStruct.functionTypeQualifier.lineNumberOfStart = lineNumber;

			//---------继续尝试查找函数参数右小括号--------------
			while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
			{
				if (*p21 == '\n')
				{
					lineNumber--;
				}
				p21--;
			}

			if (p21 <= p1)
			{
				ret = -6;
				printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
				break;
			}

			if (*p21 == ')')
			{
				funcStruct.functionParameter.end = p21;
				funcStruct.functionParameter.fileOffsetOfEnd = funcStruct.functionParameter.end - p1;
				funcStruct.functionParameter.lineNumberOfEnd = lineNumber;
			}
			else //------说明不是一个完整的函数定义，则从下一个位置重新查找--------
			{
				lineNumber = lineNumberTemp;
				p2++;
				if (*p2 == '\n')
				{
					lineNumber++;
				}
				p1 = p2; //更新 p1 的值
				goto retry;
			}
		}

		//--------查找函数参数左小括号----------------
		parenthesesCount = 0;
		p21 = funcStruct.functionParameter.end - 1;

		while (p21 >= p1)
		{
			if (*p21 == ')') //函数参数列表内部可能也含有"()"小括号对
			{
				parenthesesCount++;
			}
			else if (*p21 == '(')
			{
				if (parenthesesCount == 0) //说明找到了
				{
					break;
				}
				else
				{
					parenthesesCount--;
				}
			}

			if (*p21 == '\n')
			{
				lineNumber--;
			}
			p21--;
		}

		if (p21 <= p1)
		{
			ret = -8;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		if (*p21 == '(')
		{
			funcStruct.functionParameter.start = p21;
			funcStruct.functionParameter.fileOffsetOfStart = funcStruct.functionParameter.start - p1;
			funcStruct.functionParameter.lineNumberOfStart = lineNumber;
		}

		//--------查找函数名----------------
		p21 = funcStruct.functionParameter.start - 1;

		while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
		{
			if (*p21 == '\n')
			{
				lineNumber--;
			}
			p21--;
		}

		if (p21 <= p1)
		{
			ret = -4;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}
		
		funcStruct.functionName.end = p21;
		funcStruct.functionName.fileOffsetOfEnd = funcStruct.functionName.end - p1;
		funcStruct.functionName.lineNumberOfEnd = lineNumber;

		p21 = funcStruct.functionName.end - 1;
		while (p21 >= p1)
		{
			if (!((*p21 >= '0' && *p21 <= '9') 
				|| (*p21 >= 'a' && *p21 <= 'z') 
				|| (*p21 >= 'A' && *p21 <= 'Z') 
				|| (*p21 == '_')
				|| (*p21 == '~') //C++ 类的析构函数
				|| (*p21 == ':') //C++ 类作用域限定符"::"
				)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
			{
				break;
			}

			p21--;
		}

		if (p21 <= p1)
		{
			ret = -5;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		funcStruct.functionName.start = p21 + 1;
		funcStruct.functionName.fileOffsetOfStart = funcStruct.functionName.start - p1;
		funcStruct.functionName.lineNumberOfStart = lineNumber;

		ret2 = isKeyword(funcStruct.functionName.start, funcStruct.functionName.end - funcStruct.functionName.start + 1);
		if(ret2) //函数名是C/C++语言关键词
		{
			lineNumber = lineNumberTemp;
			p2++;
			if (*p2 == '\n')
			{
				lineNumber++;
			}
			p1 = p2; //更新 p1 的值
			goto retry;
		}

		//--------查找函返回值----------------
		p21 = funcStruct.functionName.start - 1;

		while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
		{
			if (*p21 == '\n')
			{
				lineNumber--;
			}
			p21--;
		}

		if (p21 <= p1)
		{
			ret = -6;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
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
		while (p21 >= p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
		{
			if (*p21 == '\n')
			{
				lineNumber--;
			}
			p21--;
		}

		while (p21 >= p1)
		{
			if ((*p21 >= '0' && *p21 <= '9')
				|| (*p21 >= 'a' && *p21 <= 'z')
				|| (*p21 >= 'A' && *p21 <= 'Z')
				|| (*p21 == '_') //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
				|| (*p21 == '<' || *p21 == '>') //C++ 模板 template <typename T> class A {};
				)
			{

			}
			else
			{
				if (*p21 == '{' //可能函数体在C++类的内部
					|| *p21 == '}' //可能是上一个函数结束的位置
					|| *p21 == ';' //语句结束
					|| *p21 == '）' //类似 #pragma comment(lib, "user32.lib")
					|| *p21 == '\\' //类似 '#define  MACRO_A \'
					|| *p21 == ':' //类似 class A { public: A(){} };
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

		if (p21 <= p1)
		{
			ret = -7;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		wordCount++;
		if (overSerach == 0 && wordCount < wordCountMax && lineCnt < lineCntMax)
		{
			goto retry2;
		}

		if (p21 < funcStruct.functionName.start)
		{
			funcStruct.functionReturnValue.start = p21;
			funcStruct.functionReturnValue.fileOffsetOfStart = funcStruct.functionReturnValue.start - p1;
			funcStruct.functionReturnValue.lineNumberOfStart = lineNumber;
		}

		//--------查找函数体右大括号----------------
retry3:
		lineNumber = lineNumberTemp;
		braceCount = 0;
		p2 = funcStruct.functionBody.start + 1;

		while (p2 <= p3)
		{
			if (*p2 == '{') //函数体内部可能也含有"{}"大括号对
			{
				braceCount++;
			}else if (*p2 == '}')
			{
				if (braceCount == 0) //说明找到了
				{
					break;
				}
				else
				{
					braceCount--;
				}
			}

			if (*p2 == '\n')
			{
				lineNumber++;
			}
			p2++;
		}

		if (p2 >= p3)
		{
			ret = -8;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		if (*p2 == '}')
		{
			funcStruct.functionBody.end = p2;
			funcStruct.functionBody.fileOffsetOfEnd = funcStruct.functionBody.end - p1;
			funcStruct.functionBody.lineNumberOfEnd = lineNumber;
			funcStruct.functionBody.length = funcStruct.functionBody.end - funcStruct.functionBody.start + 1;

			//--------查找函数体内部调用了哪些其他函数---------------
			ret = findAllFuncsInFunctionBody(funcStruct.functionBody.start, funcStruct.functionBody.length, funcStruct.funcsWhichInFunctionBody, p1, funcStruct.functionBody.lineNumberOfStart);
			if (ret != 0)
			{
				ret = -8;
				printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
				break;
			}
		}

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
			for(int i = 0; i < funcStruct.functionName.length - 2; ++i)
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

	//------先将被注释掉的代码用空格' '代替（行号符'\n'保留）--------
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


int CFuncRoute::findAllFuncsInFunctionBody(unsigned char *buffer, int bufferSize, std::vector<CLASS_INSTANCE> &funcsWhichInFunctionBody, unsigned char *bufferBase, int lineNumberBase)
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
	int pointerClassFlag = 0;

	//--------查找函数体内部调用了哪些其他函数---------------
retry:
	while (p2 <= p3)
	{
		if (*p2 == '(') //可能是函数调用的参数列表的左括号
		{
			p21 = p2;
			lineNumberTemp = lineNumber;

			CLASS_INSTANCE instance;
			memset(&instance, 0, sizeof(CLASS_INSTANCE));

			instance.functionArgs.start = p21;
			instance.functionArgs.fileOffsetOfStart = instance.functionArgs.start - p11;
			instance.functionArgs.lineNumberOfStart = lineNumber;

			p21--;
			while (p21 > p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
			{
				if (*p21 == '\n')
				{
					lineNumber--;
				}
				p21--;
			}

			if (p21 > p1)
			{
				instance.functionName.end = p21;
				instance.functionName.fileOffsetOfEnd = instance.functionName.end - p11;
				instance.functionName.lineNumberOfEnd = lineNumber;

				//----查找函数名----
				while (p21 >= p1)
				{
					if (!((*p21 >= '0' && *p21 <= '9')
						|| (*p21 >= 'a' && *p21 <= 'z')
						|| (*p21 >= 'A' && *p21 <= 'Z')
						|| (*p21 == '_')
						)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
					{
						break;
					}

					if (*p21 == '\n')
					{
						lineNumber--;
					}
					p21--;
				}

				if (p21 > p1) //找到函数名了
				{
					instance.functionName.start = p21 + 1;
					instance.functionName.fileOffsetOfStart = instance.functionName.start - p11;
					instance.functionName.lineNumberOfStart = lineNumber;
					instance.functionName.length = instance.functionName.end - instance.functionName.start + 1;

					instance.functionName.copyStrFromBuffer();

					//检测是否是类似 if(...) 等关键字语法，如果是，则跳过
					ret2 = isKeyword(instance.functionName.start, instance.functionName.end - instance.functionName.start + 1);
					if (ret2) //函数名是C/C++语言关键词
					{
						lineNumber = lineNumberTemp;
						p2++;
						if (*p2 == '\n')
						{
							lineNumber++;
						}
//						p1 = p2 + 1; //更新 p1 的值
						goto retry;
					}

					//-----查找该函数是否是C++类的成员函数----------
					while (p21 > p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
					{
						if (*p21 == '\n')
						{
							lineNumber--;
						}
						p21--;
					}

					if (p21 > p1)
					{
						if (*p21 == '.' //使用点号"."调用函数，例如 ret = A.set(123);
							|| *p21 == '>' //使用箭头"->"调用函数，例如 ret = A->set(123);
							)
						{
							if (*p21 == '>')
							{
								if (p21 - 1 > p1 && *(p21 - 1) == '-')
								{
									pointerClassFlag = 1;
									p21--;
								}
								else //箭头"->"中的"-"和">"之间不能有任何其他字符
								{
									ret = -8;
									printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
									goto end;
								}
							}

							p21--;

							if (p21 > p1)
							{
								while (p21 > p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
								{
									if (*p21 == '\n')
									{
										lineNumber--;
									}
									p21--;
								}

								if (p21 > p1) //找到C++类的实例名称了
								{
									instance.classInstanceName.end = p21;
									instance.classInstanceName.fileOffsetOfEnd = instance.classInstanceName.end - p11;
									instance.classInstanceName.lineNumberOfEnd = lineNumber;

									//----查找实例名称----
									while (p21 >= p1)
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

									if (p21 > p1)
									{
										instance.classInstanceName.start = p21 + 1;
										instance.classInstanceName.fileOffsetOfStart = instance.classInstanceName.start - p11;
										instance.classInstanceName.lineNumberOfStart = lineNumber;
										instance.classInstanceName.length = instance.classInstanceName.end - instance.classInstanceName.start + 1;

										instance.classInstanceName.copyStrFromBuffer();

										//-------在函数体内部尝试反向查找实例对应的类名（如果是类的成员变量则会查不到）-------------
										p21 = instance.classInstanceName.start - instance.classInstanceName.length;
										unsigned char * pTemp = p21;

										while (p21 >= p1)
										{
											if (memcmp(p21, instance.classInstanceName.start, instance.classInstanceName.length) == 0)
											{
												p21--;
												p22 = p21;
												pTemp = p21;

												int equalSignFlag = 0;
												while (p21 >= p1)
												{
													if (((*p21 >= '0' && *p21 <= '9')
														|| (*p21 >= 'a' && *p21 <= 'z')
														|| (*p21 >= 'A' && *p21 <= 'Z')
														|| (*p21 == '_')
														)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
													{
														break;
													}
													else if (*p21 == '=') //找到含有等号的语句 ret = classB.set(1); 了，但并不是类的声明语句
													{
														equalSignFlag = 1;
														break;
													}

													if (*p21 == '\n')
													{
														lineNumber--;
													}

													p21--;
												}

												if (equalSignFlag == 1)
												{
													continue;
												}

												p23 = p22;
												if (p21 > p1 && p21 < p23) //变量名和类类型之间必须至少有一个非数字字母下划线字符，例如： A *a = new A(); A b(); A &c = d; std::vector<int>e;
												{
													p23 += instance.classInstanceName.length + 1;
													
													while (p23 < instance.classInstanceName.start)
													{
														if (*p23 == ';') //C++ 变量声明必须以一个分号';'结尾
														{
															p23--;
															break;
														}
														if (*p23 == '\n')
														{
															//lineNumber++;
														}
														p23++;
													}

													if (p23 < instance.classInstanceName.start) //说明这的确是一个函数体内部声明的变量
													{
														//------继续查找变量类型--------
														while (p21 >= p1)
														{
															if (((*p21 == ';') //上一条语句结尾
																|| (*p21 == '{') //语句块开始
																|| (*p21 == '}') //语句块结束
																|| (*p21 == '：') //类似 "public: A a;"
																//|| (*p21 == ')') //类似 if(b) A a; 但这种写法，是没有任何意义的
																)) //C++ 函数名和变量命名规则，数字 + 字母 + 下划线
															{
																break;
															}

															if (*p21 == '\n')
															{
																lineNumber--;
															}

															p21--;
														}

														p21++;
														if (p21 >= p1) //找到声明的类型了
														{
															while (p21 < instance.classInstanceName.start && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
															{
																if (*p21 == '\n')
																{
																	lineNumber++;
																}
																p21++;
															}

															//----------------------------
															instance.className.start = p21;
															instance.className.fileOffsetOfStart = instance.className.start - p11;
															instance.className.lineNumberOfStart = lineNumber;

															instance.className.end = p22;
															instance.className.fileOffsetOfEnd = instance.className.end - p11;
															instance.className.lineNumberOfEnd = lineNumber;

															instance.className.length = instance.className.end - instance.className.start + 1;
															//instance.className.copyStrFromBuffer();

															int whiteSpaceFlag = 0;
															int pos = 0;

															while (p21 <= p22)
															{
																if (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n') //空白字符
																{
																	whiteSpaceFlag = 1;
																}
																else
																{
																	if (pointerClassFlag == 1 && *p21 == '*')
																	{
																		break;
																	}

																	if (pos < sizeof(instance.className.str) - 2)
																	{
																		if (whiteSpaceFlag == 1)
																		{
																			whiteSpaceFlag = 0;
																			instance.className.str[pos] = ' '; //多个连续空白字符，用一个空格代替
																			pos++;
																		}
																		instance.className.str[pos] = *p21;
																		pos++;
																	}
																}

																if (*p21 == '\n')
																{
																	lineNumber++;
																}

																p21++;
															}

															instance.className.str[pos] = '\0';
															break; //在函数体内部找到函数声明了，就直接退出循环
														}
													}
												}

												p21 = pTemp;
											}
											p21--;
										}
									}
								}
							}
						}
						else if (*p21 == ':') //使用箭头"::"调用函数，例如 ret = A::set(123);
						{
							if (p21 - 1 > p1 && *(p21 - 1) == ':')
							{
								p21 -= 2;
							}
							else //箭头"::"中的":"和":"之间不能有任何其他字符
							{
								ret = -8;
								printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
								goto end;
							}

							if (p21 > p1)
							{
								while (p21 > p1 && (*p21 == ' ' || *p21 == '\t' || *p21 == '\r' || *p21 == '\n')) //跳过空白字符
								{
									if (*p21 == '\n')
									{
										lineNumber--;
									}
									p21--;
								}

								if (p21 > p1) //找到C++类名了
								{
									instance.className.end = p21;
									instance.className.fileOffsetOfEnd = instance.className.end - p11;
									instance.className.lineNumberOfEnd = lineNumber;

									//----查找类名----
									while (p21 >= p1)
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

									if (p21 > p1)
									{
										instance.className.start = p21 + 1;
										instance.className.fileOffsetOfStart = instance.className.start - p11;
										instance.className.lineNumberOfStart = lineNumber;
										instance.className.length = instance.className.end - instance.className.start + 1;

										instance.className.copyStrFromBuffer();
									}
								}
							}
						}
					}
				}
			}

			//---------查找函数调用的参数列表的右括号-----------
			p21 = instance.functionArgs.start;
			lineNumber = lineNumberTemp;

			while (p21 < p3 && *p21 != ')')
			{
				if (*p21 == '\n')
				{
					lineNumber++;
				}
				p21++;
			}

			if (p21 <= p3)
			{
				instance.functionArgs.end = p21;
				instance.functionArgs.fileOffsetOfEnd = instance.functionArgs.end - p11;
				instance.functionArgs.lineNumberOfEnd = lineNumber;
				instance.functionArgs.length = instance.functionArgs.end - instance.functionArgs.start + 1;
				instance.functionArgs.copyStrFromBuffer();

				funcsWhichInFunctionBody.push_back(instance);
			}

			p2 = p21; //更新 p2 的值
		}
		
		if (*p2 == '\n')
		{
			lineNumber++;
		}

		p2++;
	}

end:

	return ret;
}


int CFuncRoute::macroExpand()
{
	int ret = 0;

	return ret;
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

					while (p21 <= p2)
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

					if (p21 <= p2 && p21 > p22)
					{
						MY_STRING myStrParentClass;
						memset(&myStrParentClass, 0, sizeof(MY_STRING));

						int len = MIN(p21 - p22, sizeof(myStrParentClass.str) - 1);
						memcpy(myStrParentClass.str, p22, len);
						myStrParentClass.str[len] = '\0';

						classParents.push_back(myStrParentClass);
					}
				}
			}
		}

		p2++;
	}

	return ret;
}


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
				std::string classInstanceName1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].classInstanceName.str;
				std::string className1 = vFunctions[i].funcs[j].funcsWhichInFunctionBody[k].className.str;
				std::string className12 = vFunctions[i].funcs[j].className;
				std::string classNameAlias12 = vFunctions[i].funcs[j].classNameAlias;

				if (className1 == "") //实例没有填充类名的情况下，才遍历查找
				{
					for (int i2 = 0; i2 < len1; ++i2)
					{
						int len4 = vFunctions[i2].classes.size();
						for (int j2 = 0; j2 < len4; ++j2)
						{
							int len4 = vFunctions[i2].classes[j2].memberVars.size();
							for (int m2 = 0; m2 < len4; ++m2)
							{
								std::string varName = vFunctions[i2].classes[j2].memberVars[m2].varName.str;
								std::string className41 = vFunctions[i2].classes[j2].className.str;

								if (varName == classInstanceName1) //C++类的成员变量和成员函数体中的某个变量相等了
								{
									//--------尝试查找变量是否在本类或者父类中声明的，如果不是，则可能是全局变量----------------
									int flag = 0;

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
									}
								}
							}
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
						if (j == 14 && k == 3)
						{
							int a = 1;
						}
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

	//--------打印统计信息-------------------
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
