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
	int len31 = allFuncs.size();

	for (int i = 0; i < len31; ++i)
	{
		int len32 = allFuncs[i].funcs.size();
		for (int j = 0; j < len32; ++j)
		{
			
		}
	}

	return 0;
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

	FUNCTIONS funcs;
	FUNCTION_STRUCTURE funcStruct;

	memset(&funcs, 0, sizeof(FUNCTIONS));
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

	std::vector<CLASS_STRUCT> classes;

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
								if (*p21 == ':') //说明继承自父类，类似 class B : public A{};
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

										classes.push_back(cs);

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

			//--------查找函数体内部调用了哪些其他函数---------------
			p21 = funcStruct.functionBody.start;
			std::vector<unsigned char *> vPointers;

			while(p21 < p2)
			{
				if(*p21 == '(') //可能是函数调用的参数列表的左括号
				{
					vPointers.push_back(p21);
				}else if(*p21 == ')') //可能是函数调用的参数列表的右括号
				{
					
				}

				p21++;
			}

			//-----------------------
			p1 = p2; //更新 p1 的值
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
		for(int i = 0; i < classes.size(); ++i)
		{
			if(funcStruct.functionName.start > classes[i].classBody.start && funcStruct.functionName.end < classes[i].classBody.end)
			{
				if(classes[i].isStruct) //说明是结构体的成员函数
				{
					int len = MIN(classes[i].className.length, sizeof(funcStruct.className) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.structName, classes[i].className.str, len);
						funcStruct.structName[len] = '\0';
					}

					len = MIN(classes[i].classNameAlias.length, sizeof(funcStruct.classNameAlias) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.classNameAlias, classes[i].classNameAlias.str, len);
						funcStruct.classNameAlias[len] = '\0';
					}
				}else //说明是C++类的成员函数
				{
					int len = MIN(classes[i].className.length, sizeof(funcStruct.className) - 1);
					if(len > 0)
					{
						memcpy(funcStruct.className, classes[i].className.str, len);
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
		funcs.funcs.push_back(funcStruct);
	}

end:
	//----------------
	if (buffer2){ free(buffer2); buffer2 = NULL; }

	functions = funcs;

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
							else
							{
								ret = -6;
								printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
								break;
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
				else
				{
					ret = -6;
					printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
					break;
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

