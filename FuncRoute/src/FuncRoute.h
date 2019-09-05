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
}STRING_POSITON;


typedef struct _FUNCTION_STRUCTURE_
{
	STRING_POSITON functionReturnValue; //函数返回值
	STRING_POSITON functionName; //函数名
	STRING_POSITON functionParameter; //函数参数
	STRING_POSITON functionBody; //函数体
	char funcString[1024]; //函数返回值 + 函数名 + 函数参数
}FUNCTION_STRUCTURE;


typedef struct _FUNCTIONS_
{
	unsigned char fllename[600]; //所在文件名
	std::vector<FUNCTION_STRUCTURE> funcs;
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
