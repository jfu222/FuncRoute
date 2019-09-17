#include <stdio.h>
#include "FuncRoute.h"
#include "version.h"


int print_help(int argc, char *argv[])
{
	printf("====== Function-Route Version: %s ======\n\n", VERSION_STR3(VERSION_STR));

	printf("Usage:\n");
	printf("%s <in|dirs_include> [in|dirs_exclude]\n", argv[0]);
	printf("For Example:\n");
	printf("%s ./data1/src;./data2\n", argv[0]);
	printf("%s ./data1/src;./data2 ./data1/src/include;./data2/include\n", argv[0]);

	return 0;
}


int main(int argc, char *argv[])
{
	CFuncRoute fr;

	int ret = 0;

	if (argc != 2 && argc != 3)
	{
		print_help(argc, argv);
		return -1;
	}

	std::string dirs = "C:\\vs2013\\test;C:\\vs2013\\test2";
	std::vector<std::string> fileDirsInclude;
	std::vector<std::string> fileDirsExclude;
	std::vector<std::string> suffixes;

	//--------------
	dirs = argv[1];
	ret = fr.splitDirsBySemicolon(dirs, fileDirsInclude);
	if (ret != 0)
	{
		return -1;
	}

	//--------------
	if (argc == 3)
	{
		dirs = argv[2];
		ret = fr.splitDirsBySemicolon(dirs, fileDirsExclude);
		if (ret != 0)
		{
			return -1;
		}
	}

	suffixes.push_back(".h");
	suffixes.push_back(".hpp");
	suffixes.push_back(".c");
	suffixes.push_back(".cpp");
	suffixes.push_back(".cc");
//	suffixes.push_back("*");

	ret = fr.findAllFunctionsName(fileDirsInclude, fileDirsExclude, suffixes);
	
	printf("fr.findAllFunctionsName(): ret=%d;\n", ret);

	return 0;
}