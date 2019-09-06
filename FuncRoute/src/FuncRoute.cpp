#include "FuncRoute.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./os/share_library.h"


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

		//-----------------
		for (int i = 0; i < len2; ++i)
		{
			printf("[%d/%d] %s; line=%d;\n", i + 1, len2, functions.funcs[i].funcString, functions.funcs[i].functionName.lineNumberOfStart);
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
	unsigned int lineNumber_temp = 1;
	unsigned char *p11 = NULL;
	unsigned char *p21 = NULL;
	unsigned char *p22 = NULL;
	int braceCount = 0; //大括号对计数
	int parenthesesCount = 0; //小括号对计数
	int lenFuncString = sizeof(funcStruct.funcString);

	int wordCountMax = 5; //函数名前面，最多搜索5个以空格隔开的字符串
	int wordCount = 0;
	int overSerach = 0; //停止搜索
	int lineCntMax = 5; //函数名前面，最多搜索5行
	int lineCnt = 0;

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

			}
		}

		p2++;
	}

	//---------------------------
	p2 = buffer2;
	p21 = NULL;

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
			funcStruct.functionBody.lineNumberOfStart = lineNumber;
		}

		//--------查找函数参数右小括号----------------
		lineNumber_temp = lineNumber;
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
				funcStruct.functionParameter.lineNumberOfEnd = lineNumber;
			}
			else //------说明不是一个完整的函数定义，则从下一个位置重新查找--------
			{
				lineNumber = lineNumber_temp;
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
			funcStruct.functionParameter.lineNumberOfStart = lineNumber;
		}

		//--------查找函名----------------
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
		funcStruct.functionName.lineNumberOfStart = lineNumber;

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
			funcStruct.functionReturnValue.lineNumberOfStart = lineNumber;
		}

		//--------查找函数体右大括号----------------
retry3:
		lineNumber = lineNumber_temp;
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
			funcStruct.functionBody.lineNumberOfEnd = lineNumber;

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

			if (funcStruct.functionName.length > 0)
			{
				memcpy(pTemp, funcStruct.functionReturnValue.start, funcStruct.functionReturnValue.length);
				pTemp += funcStruct.functionReturnValue.length;
			}

			if (funcStruct.functionName.length > 0)
			{
				*pTemp = ' ';
				pTemp++;
				memcpy(pTemp, funcStruct.functionName.start, funcStruct.functionName.length);
				pTemp += funcStruct.functionName.length;
			}

			if (funcStruct.functionParameter.length > 0)
			{
				*pTemp = ' ';
				pTemp++;
				memcpy(pTemp, funcStruct.functionParameter.start, funcStruct.functionParameter.length);
				pTemp += funcStruct.functionParameter.length;
			}

			if (funcStruct.functionTypeQualifier.length > 0)
			{
				*pTemp = ' ';
				pTemp++;
				memcpy(pTemp, funcStruct.functionTypeQualifier.start, funcStruct.functionTypeQualifier.length);
				pTemp += funcStruct.functionTypeQualifier.length;
			}
			*pTemp = '\0';
		}
		else
		{
			ret = -9;
			printf("%s(%d): %s: Error: ret=%d; lineNumber=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret, lineNumber);
			break;
		}

		funcs.funcs.push_back(funcStruct);
	}

end:
	//----------------
	if (buffer2){ free(buffer2); buffer2 = NULL; }

	functions = funcs;

	return 0;
}
