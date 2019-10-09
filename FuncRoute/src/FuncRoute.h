#ifndef __FUNC_ROUTE_H__
#define __FUNC_ROUTE_H__

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <stack>
#include "CommonData.h"


//---------C/C++源代码文件函数调用关系类-----------------
class CFuncRoute
{
public:
	std::string m_srcCodesFilePath; //C/C++源代码文件路径
	std::vector<std::string> m_fileSuffixes; //C/C++源代码文件后缀名（忽略大小写）数组，例如 [".h", ".hpp", ".c", ".cpp", ".cc", "*"]
	std::string m_filePathForPdfTex; //用于生成pdf的tex文件，生成命令为“pdflatex ./out_FuncRoute.tex”，会在当前目录生成./out_FuncRoute.pdf文件

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
	int findCurLineStartAndEndPos(unsigned char *buffer, int bufferSize, unsigned char *curPos, unsigned char *&startPos, unsigned char *&endPos);
	int isBetweenInDoubleQuotes(unsigned char *buffer, int bufferSize, unsigned char *curPos, unsigned char *&startPos, unsigned char *&endPos); //当前字符是否处于一对双引号之间
	int isBetweenInSingleQuotes(unsigned char *buffer, int bufferSize, unsigned char *curPos, unsigned char *&startPos, unsigned char *&endPos); //当前字符是否处于一对单引号之间
	int replaceAllStrBySpace(unsigned char *buffer, int bufferSize); //将所有用双引号""的代码用反单引号'`'代替
	int replaceAllMacroDefineStrBySpace(unsigned char *buffer, int bufferSize); //将所有#define宏定义用空格' '代替
	int findStr(unsigned char *buffer, int bufferSize, const char *str, int &pos); //在内存中，查找指定的字符串
	int findAllMacros(std::vector<std::string> files, std::vector<MACRO> &macros); //从所有代码源文件中，找到所有的宏定义
	int findAllClassAndStructDeclare(unsigned char *buffer, int bufferSize, std::vector<CLASS_STRUCT> &classes); //在内存中查找所有完整的C++类/结构体声明
	int findAllFuncsInFunctionBody(unsigned char *buffer, int bufferSize, char *funcParameter, std::vector<CLASS_INSTANCE> &funcsWhichInFunctionBody, unsigned char *bufferBase, int lineNumberBase); //查找函数体内部调用的所有其他函数
	int findWholeFuncCalled(unsigned char *buffer, int bufferSize, char *funcParameter, unsigned char *parentheseLeft, CLASS_INSTANCE &classInstance, unsigned char *bufferBase, int lineNumberBase); //给定左小括号'('的位置，返回一个完整的函数调用
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
	bool isValidVarName(unsigned char *buffer, int bufferSize); //是否是一个有效的变量命名，C++ 函数名和变量命名规则，数字 + 字母 + 下划线
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

	int createPdfTexHeader(std::string &strTexHeader); //生成 test.tex头部 ，可转换成 test.pdf
	int createPdfTexLogo(std::string &strTexlogo); //生成 test.tex logo ，可转换成 test.pdf
	int createPdfTexBody(std::vector<FUNCTIONS> &vFunctions, _FUNC_INDEX_ *rootNode, std::string &strTexBody, FILE *fp, long long &writeBytes, 
		int colMax, int rowMax, int colBase, int rowBase, std::vector<FUNC_INDEX_POS> &vecNodes); //生成 test.tex身体，转换成 test.pdf
	int createPdfTexBodySub(std::vector<FUNCTIONS> &vFunctions, FILE *fp, long long &writeBytes, int colMax, int rowMax, int colBase, int rowBase, std::vector<FUNC_INDEX_POS> &vecNodes); //生成 test.tex身体，转换成 test.pdf
	int createPdfTexTailer(std::string &strTexTailer); //生成 test.tex尾部，可转换成 test.pdf
	int getTexFuncName(std::vector<FUNCTIONS> &vFunctions, _FUNC_INDEX_ *node, bool isRecursiveFunction, int cloNum, int rowNum, std::string &strFuncName);
	
	int getBuildDate1(char *szBuildDate);
	int getBuildDate2(char *szBuildDate);
	int printDeltaTime(long long timeStart, long long timeEnd);
};

#endif //__FUNC_ROUTE_H__
