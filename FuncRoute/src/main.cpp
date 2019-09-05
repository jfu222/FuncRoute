#include <stdio.h>
#include "FuncRoute.h"


int main(int argc, char *argv[])
{
	CFuncRoute fr;

	int ret = 0;

//	std::string filePath = "F:\\work\\vs2013\\test";
	std::string filePath = "F:\\git\\DuoXiangShi";

	std::vector<std::string> suffixes;

	suffixes.push_back(".h");
	suffixes.push_back(".hpp");
	suffixes.push_back(".c");
	suffixes.push_back(".cpp");
	suffixes.push_back(".cc");
//	suffixes.push_back("*");

	ret = fr.findAllFunctionsName(filePath, suffixes);

	printf("fr.findAllFunctionsName(): ret=%d;\n", ret);

	return 0;
}
