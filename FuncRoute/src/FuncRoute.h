#ifndef __FUNC_ROUTE_H__
#define __FUNC_ROUTE_H__

#include <string>
#include <vector>


typedef struct _BYTES_STREAM_
{
	unsigned char *buffer;
	unsigned int bufferSize;
	unsigned int offset;
}BYTES_STREAM;


typedef struct _STRING_POSITON_
{
	unsigned char *start;
	unsigned char *end;
	unsigned int lineNumberOfStart;
	unsigned int lineNumberOfEnd;
	unsigned int length;
	char str[1024];
/*
public:
	_STRING_POSITON_()
	{
		start = NULL;
		end = NULL;
		lineNumberOfStart = 0;
		lineNumberOfEnd = 0;
		length = 0;
		str = NULL;
	}
	_STRING_POSITON_(const _STRING_POSITON_ &s)
	{
		*this = s;
	}
	~_STRING_POSITON_()
	{
		freeData();
	}
	_STRING_POSITON_ operator = (const _STRING_POSITON_ &s)
	{
		this->start = s.start;
		this->end = s.end;
		this->lineNumberOfStart = s.lineNumberOfStart;
		this->lineNumberOfEnd = s.lineNumberOfEnd;
		this->length = s.length;
		this->str = NULL;

		this->copyStrFromBuffer();

		return *this;
	}
	int freeData(){ if(str){free(str); str = NULL;} return 0; }
	int copyStrFromBuffer()
	{
		int ret = 0;
		if (start == NULL){ return -1; }
		ret = freeData();
		str = (char *)malloc(length + 1);
		if (str == NULL){ printf("Error: malloc() failed!\n"); return -1; }
		memcpy(str, start, length);
		str[length] = '\0';
		return 0;
	}*/
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


typedef struct _FUNCTION_STRUCTURE_
{
	STRING_POSITON functionReturnValueTypeQualifier; //函数返回值(type-qualifier类型限定符)： const，template, virtual, inline, static, extern, explicit, friend, constexpr
	STRING_POSITON functionReturnValue; //函数返回值类型(type-specifier类型区分符): void *, void, int, short, long, float, double , auto, struct结构体类型，enum枚举类型，typedef类型
	STRING_POSITON functionName; //函数名
	STRING_POSITON functionParameter; //函数参数
	STRING_POSITON functionTypeQualifier; //函数参数右小括号后面紧跟的修饰符(type-qualifier类型限定符)：=0, =default, =delete, const，voliate, &(左值引用限定符), &&(右值引用限定符), override, final, noexcept, throw
	STRING_POSITON functionBody; //函数体
	char funcString[1024]; //函数返回值 + 函数名 + 函数参数
/*
public:
	_FUNCTION_STRUCTURE_(){}
	~_FUNCTION_STRUCTURE_(){}
	_FUNCTION_STRUCTURE_(const _FUNCTION_STRUCTURE_ &s)
	{
		*this = s;
	}
	_FUNCTION_STRUCTURE_ operator = (const _FUNCTION_STRUCTURE_ &f)
	{
		this->functionReturnValueTypeQualifier = f.functionReturnValueTypeQualifier;
		this->functionReturnValue = f.functionReturnValue;
		this->functionName = f.functionName;
		this->functionParameter = f.functionParameter;
		this->functionTypeQualifier = f.functionTypeQualifier;
		this->functionBody = f.functionBody;

		memcpy(this->funcString, f.funcString, sizeof(funcString));

		return *this;
	}*/
}FUNCTION_STRUCTURE;


typedef struct _FUNCTIONS_
{
	unsigned char fllename[600]; //所在文件名
	std::vector<FUNCTION_STRUCTURE> funcs;
/*
public:
	_FUNCTIONS_(){}
	~_FUNCTIONS_(){}
	_FUNCTIONS_ operator = (const _FUNCTIONS_ &f)
	{
		memcpy(this->fllename, f.fllename, sizeof(fllename));

		int len = f.funcs.size();
		for (int i = 0; i < len; ++i)
		{
			this->funcs.push_back(f.funcs[i]);
		}

		return *this;
	}*/
}FUNCTIONS;


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
};

#endif //__FUNC_ROUTE_H__
