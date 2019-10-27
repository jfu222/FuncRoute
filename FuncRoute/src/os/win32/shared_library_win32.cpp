#if defined(_WIN32) || defined(_WIN64)

#include "../share_library.h"
#include <windows.h>
#include <stdio.h>
#include <Psapi.h>
#include <time.h>
#include <io.h>
#include <Shlwapi.h>


share_library_handle share_library_load(const char *file_name)
{
	if(!file_name){return NULL;}
	return (share_library_handle)LoadLibraryA(file_name);
}


func_pointer share_library_get_func_addr(share_library_handle handle, const char *func_name)
{
	if(!handle){return NULL;}
	return (func_pointer)GetProcAddress((HMODULE)handle, func_name);
}


int share_library_free(share_library_handle handle)
{
	if(!handle){return 0;}
	return (FreeLibrary((HMODULE)handle) == TRUE) ? 0 : -1;
}


//--------------------------------------------------
int get_exe_dir_path(char *dir_path, int size)
{
	int ret = 0;
	HMODULE hMoudule = NULL;

	if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "get_exe_file_path", &hMoudule))
	{
		ret = GetLastError();
		printf("Error: GetModuleHandleExA failed; ret=%d\n", ret);
		return -1;
	}

	ret = GetModuleFileNameA(hMoudule, dir_path, size);
	if(!ret)
	{
		ret = GetLastError();
		printf("Error: GetModuleFileNameA failed; ret=%d\n", ret);
		return -2;
	}

	char * p = strrchr(dir_path, '\\');
	if(!p){return -3;}
	
	p[0] = '\0';

	int len = strlen(dir_path);

	return len;
}


int get_children_dir_name(char *parent_dir_path, std::vector<std::string> &children_dir_name)
{
	std::vector<std::string> vector_files;

	std::string folder = parent_dir_path;
	std::string folder_match = folder + "\\*.*";
	
	char out[600];
	int cnt = 0;
	long total_time = 0;

	//-----------遍历目录下--------------------
	WIN32_FIND_DATA w32FindData = {0};
	HANDLE hFindHandle = FindFirstFile(folder_match.c_str(), &w32FindData);
	if(hFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string strFileName = w32FindData.cFileName;
			if(strFileName == "." || strFileName == ".."){continue;}

			if(w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				children_dir_name.push_back(strFileName);
			}
		}while(FindNextFile(hFindHandle, &w32FindData));
		FindClose(hFindHandle);
	}

	return 0;
}


int get_dir_files(const char *dir_path, std::vector<std::string> &files)
{
	std::vector<std::string> vector_files;

	std::string folder = dir_path;
	std::string folder_match = folder + "*.*";

	char out[600];
	int cnt = 0;
	long total_time = 0;

	//-------------------------------
	WIN32_FIND_DATA w32FindData = { 0 };
	HANDLE hFindHandle = FindFirstFile(folder_match.c_str(), &w32FindData);
	if (hFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string strFileName = w32FindData.cFileName;
			if (strFileName == "." || strFileName == ".."){ continue; }

			if (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//do nothing
			}
			else
			{
				files.push_back(strFileName);
			}
		} while (FindNextFile(hFindHandle, &w32FindData));
		FindClose(hFindHandle);
	}

	return 0;
}


int get_nested_dir_files(const char *dir_path, std::vector<std::string> &files)
{
	std::vector<std::string> vector_files;

	std::string folder = dir_path;
	if (folder.substr(folder.length() - 1, 1) != "\\")
	{
		folder += "\\";
	}

	std::string folder_match = folder + "*.*";

	char out[600];
	int cnt = 0;
	long total_time = 0;

	//-------------------------------
	WIN32_FIND_DATA w32FindData = { 0 };
	HANDLE hFindHandle = FindFirstFile(folder_match.c_str(), &w32FindData);
	if (hFindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::string strFileName = w32FindData.cFileName;
			if (strFileName == "." || strFileName == ".."){ continue; }

			if (w32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				strFileName = folder + std::string(w32FindData.cFileName) + "\\";
				get_nested_dir_files(strFileName.c_str(), files);
			}
			else
			{
				strFileName = folder + std::string(w32FindData.cFileName);
				files.push_back(strFileName);
			}
		} while (FindNextFile(hFindHandle, &w32FindData));
		FindClose(hFindHandle);
	}

	return 0;
}


int get_file_dirname_and_basename_and_extname(const char *file_name, std::string &dir_name, std::string &base_name, std::string &extension_name)
{
	if(file_name == NULL || strlen(file_name) <= 0)
	{
		return -1;
	}

	std::string filename = file_name;

	size_t pos1 = filename.rfind('.');
	size_t pos2 = filename.rfind('\\');

	if(pos1 == std::string::npos)
	{
		extension_name = "";
	}else
	{
		extension_name = filename.substr(pos1 + 1);
	}
	
	if(pos2 == std::string::npos)
	{
		dir_name = "";
		base_name = filename;
	}else
	{
		dir_name = filename.substr(0, pos2);
		base_name = filename.substr(pos2 + 1);
	}

	return 0;
}


int set_dll_directory(const char *dir_path)
{
	BOOL bRet = SetDllDirectoryA(dir_path);
	return (bRet) ? 0 : -1;
}


bool is_file_exist(const char *file_path)
{
	bool ret = false;

	if(_access(file_path, 0) == 0)
	{
		ret = true;
	}

	return ret;
}


int print_mem_usage()
{
	int ret = 0;

	HANDLE hProcess = GetCurrentProcess();

	PROCESS_MEMORY_COUNTERS_EX pmc;
	pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
	
	BOOL ret2 = GetProcessMemoryInfo(hProcess, (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc));
	
	SIZE_T mem = pmc.WorkingSetSize;

	printf("cpu mem usage: %ld bytes = %0.2f KB = %0.2f MB = %0.2f GB\n", mem, mem / 1024.0, mem / (1024.0 * 1024.0), mem / (1024.0 * 1024.0 * 1024.0));

	return ret;
}


int print_date_time(const char *str)
{
	char str_time[80];
	struct tm newtime;
	time_t long_time = time(NULL);
	errno_t err = _localtime64_s(&newtime, &long_time);
	strftime(str_time, 80, "[%Y-%m-%d %H:%M:%S]", &newtime);

	if(str)
	{
		printf("%s %s\n", str_time, str);
	}else
	{
		printf("%s\n", str_time);
	}

	return 0;
}


unsigned long get_current_thread_id()
{
	DWORD id = GetCurrentThreadId();

	return id;
}


int create_nested_dir(const char *dir) //创建嵌套目录
{
	char dir1[600] = { 0 };
	char dir2[600] = { 0 };
	int flag = 0;
	BOOL bRet = FALSE;

	strcpy(dir1, dir);

	int len = strlen(dir1);
	if (len <= 3) // 如果类似 "D:\" 则返回错误
	{
		return -1;
	}

	if (dir1[len - 1] != '\\')
	{
		dir1[len] = '\\';
		dir1[len + 1] = '\0';
		len++;
	}

	for (int i = 0; i < len; ++i)
	{
		if (dir1[i] == '\\')
		{
			if (flag == 1)
			{
				bRet = PathFileExistsA(dir2);
				if (bRet == FALSE)
				{
					bRet = CreateDirectoryA(dir2, NULL);
					if (bRet == FALSE)
					{
//						DWORD error = GetLastError(); //ERROR_ALREADY_EXISTS
						printf("%s: Cannot create dir '%s'\n", __FUNCTION__, dir2);
						return -1;
					}
//					bRet = SetFileAttributes(dir2, FILE_ATTRIBUTE_NORMAL);
				}
			}
			dir2[i] = dir1[i];
			flag = 1;
		}
		else
		{
			dir2[i] = dir1[i];
		}
	}

	return 0;
}

#endif // #if defined(_WIN32) || defined(_WIN64)
