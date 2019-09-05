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

	char whiteSpace[] = {' ', '\t', '\r', '\n'}; //空白字符
	char scopeResolutionOperator[] = "::"; //C++ 作用域限定符
	char varName[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"; //C++ 变量命名规则，数字 + 字母 + 下划线

	FUNCTIONS funcs;
	FUNCTION_STRUCTURE funcStruct;

	memset(&funcs, 0, sizeof(FUNCTIONS));
	memset(&funcStruct, 0, sizeof(FUNCTION_STRUCTURE));

	unsigned char *p1 = buffer;
	unsigned char *p2 = buffer;
	unsigned char *p3 = buffer + bufferSize;
	unsigned int lineNumber = 1;
	unsigned int lineNumber_temp = 1;
	unsigned char *p11 = NULL;
	unsigned char *p21 = NULL;
	int braceCount = 0; //大括号对计数
	int parenthesesCount = 0; //小括号对计数
	int lenFuncString = sizeof(funcStruct.funcString);

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
			p2++;
			if (*p2 == '\n')
			{
				lineNumber++;
			}
			goto retry;
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
			break;
		}

		funcStruct.functionReturnValue.end = p21;
		funcStruct.functionReturnValue.lineNumberOfEnd = lineNumber;

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

		funcStruct.functionReturnValue.start = p21 + 1;
		funcStruct.functionReturnValue.lineNumberOfStart = lineNumber;

		//--------查找函数体右大括号----------------
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
		}

		//--------查找到一个完整的函数----------------
		funcStruct.functionReturnValue.length = funcStruct.functionReturnValue.end - funcStruct.functionReturnValue.start + 1;
		funcStruct.functionName.length = funcStruct.functionName.end - funcStruct.functionName.start + 1;
		funcStruct.functionParameter.length = funcStruct.functionParameter.end - funcStruct.functionParameter.start + 1;
		funcStruct.functionBody.length = funcStruct.functionBody.end - funcStruct.functionBody.start + 1;

		if (funcStruct.functionReturnValue.length + 1 + funcStruct.functionName.length + 1 + funcStruct.functionParameter.length < lenFuncString)
		{
			char * pTemp = funcStruct.funcString;

			memcpy(pTemp, funcStruct.functionReturnValue.start, funcStruct.functionReturnValue.length);
			pTemp += funcStruct.functionReturnValue.length;
			*pTemp = ' ';
			pTemp++;

			memcpy(pTemp, funcStruct.functionName.start, funcStruct.functionName.length);
			pTemp += funcStruct.functionName.length;
			*pTemp = ' ';
			pTemp++;

			memcpy(pTemp, funcStruct.functionParameter.start, funcStruct.functionParameter.length);
			pTemp += funcStruct.functionParameter.length;
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

	functions = funcs;

	return 0;
}
