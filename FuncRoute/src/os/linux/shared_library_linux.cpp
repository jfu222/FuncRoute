#if !defined(_WIN32) && !defined(_WIN64)

#include "../share_library.h"
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#define MAX_PATH        260


share_library_handle share_library_load(const char *file_name)
{
	if(!file_name){return NULL;}
	return (share_library_handle)dlopen(file_name, RTLD_LAZY);
}


func_pointer share_library_get_func_addr(share_library_handle handle, const char *func_name)
{
	if(!handle){return NULL;}
	return (func_pointer)dlsym(handle, func_name);
}


int share_library_free(share_library_handle handle)
{
	if(!handle){return 0;}
	return dlclose(handle);
}


//--------------------------------------------------
int get_exe_dir_path(char *dir_path, int size)
{
	int ret = 0;

	ret = readlink("/proc/self/exe", dir_path, size);
	if(ret < 0 || ret >= size)
	{
		printf("Error: readlink failed!\n");
		return -1;
	}

	dir_path[ret] = '\0';
	
	char * p = strrchr(dir_path, '/');
	if(!p){return -3;}
	
	p[0] = '\0';

	int len = strlen(dir_path);

	return len;
}


int get_children_dir_name(char *parent_dir_path, std::vector<std::string> &children_dir_name)
{
	struct dirent * pDirEntry = NULL;

	DIR * pDir = opendir(parent_dir_path);

	if(pDir == NULL)
	{
		printf("Error: opendir failed! %s;\n", parent_dir_path);
		return -1;
	}

	while(pDirEntry = readdir(pDir))
	{
		std::string strFileName = pDirEntry->d_name;
		if(strFileName == "." || strFileName == ".."){continue;}

		children_dir_name.push_back(strFileName);
	}

	closedir(pDir);

	return 0;
}


int get_dir_files(const char *dir_path, std::vector<std::string> &files)
{
	struct dirent * pDirEntry = NULL;

	DIR * pDir = opendir(dir_path);

	if (pDir == NULL)
	{
		printf("Error: opendir failed! %s;\n", dir_path);
		return -1;
	}

	while (pDirEntry = readdir(pDir))
	{
		std::string strFileName = pDirEntry->d_name;
		if (pDirEntry->d_type & DT_DIR)
		{
			if (strFileName == "." || strFileName == ".."){ continue; }
		}
		else //DT_REG
		{
//			printf("%s\n", strFileName.c_str());
			files.push_back(strFileName);
		}
	}

	closedir(pDir);

	return 0;
}


int get_nested_dir_files(const char *dir_path, std::vector<std::string> &files)
{
	struct dirent * pDirEntry = NULL;

	std::string folder = dir_path;
	if (folder.substr(folder.length() - 1, 1) != "/")
	{
		folder += "/";
	}

	DIR * pDir = opendir(dir_path);

	if (pDir == NULL)
	{
		printf("Error: opendir failed! %s;\n", dir_path);
		return -1;
	}

	while (pDirEntry = readdir(pDir))
	{
		std::string strFileName = pDirEntry->d_name;
		if (pDirEntry->d_type & DT_DIR)
		{
			if (strFileName == "." || strFileName == ".."){ continue; }

			strFileName = folder + std::string(w32FindData.cFileName) + "/";
			get_nested_dir_files(strFileName.c_str(), files);
		}
		else //DT_REG
		{
			strFileName = folder + std::string(w32FindData.cFileName);
//			printf("%s\n", strFileName.c_str());
			files.push_back(strFileName);
		}
	}

	closedir(pDir);

	return 0;
}


int set_dll_directory(const char *dir_path)
{
	return 0;
}


bool is_file_exist(const char *file_path)
{
	bool ret = false;
	
	if(access(file_path, 0) == 0)
	{
		ret = true;
	}

	return ret;
}


/*----------------/proc/7822/status---------------------------
Name:   java
State:  S (sleeping)
Tgid:   7822
Ngid:   0
Pid:    7822
PPid:   1
TracerPid:      0
Uid:    0       0       0       0
Gid:    0       0       0       0
FDSize: 256
Groups: 0
VmPeak:  7844872 kB
VmSize:  7844856 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:   1157060 kB
VmRSS:   1138484 kB
VmData:  7687708 kB
VmStk:       136 kB
VmExe:         4 kB
VmLib:     17036 kB
VmPTE:      2708 kB
VmSwap:        0 kB
Threads:        97
...
*/
int print_mem_usage()
{
	int ret = 0;

	pid_t p = getpid();

	char file[260] = {0};
	char buffer[4096] = {0};

	sprintf(file, "/proc/%d/status", p); // /proc/self/status
	FILE * fp = fopen(file, "rb");
	if(fp == NULL){printf("Error: cannot open file %s\n", file); return -1;}

	int len = fread(buffer, 1, sizeof(buffer) - 1, fp);
	fclose(fp);

	buffer[len] = '\0';
	
	//---------------------------
	char VmRSS[50] = {0};
	for(int i = 0; i < len; i++)
	{
		if(buffer[i] == '\n')
		{
			if(strncmp(buffer + i + 1, "VmRSS:", 6) == 0)
			{
				int j = i + 7;
				while(j < len && buffer[j] != '\n'){j++;}
				
				int len2 = j - i - 1;
				if(len2 > 50){len2 = 50;}
				
				memcpy(VmRSS, buffer + i + 1, len2);
				VmRSS[len2] = '\0';
				break;
			}
		}
	}
	
	//---------------------------
	//USER       PID  %CPU  %MEM    VSZ     RSS   TTY      STAT START   TIME COMMAND
	//root       1    0.0   0.0     125116  3948  ?        Ss   Jul31   0:03 /usr/lib/systemd/systemd --switched-root --system --deserialize 21
	char buffer2[100] = {0};
	char str_cmd[100] = "";
	
	sprintf(str_cmd, "ps aux | awk '$2 == %d{printf(\"CPU=\"$3\"%%%%, MEM=\"$4\"%%%%, VSZ=\"$5\" KB, RSS=\"$6\" KB, CMD=\"$11);}'", p);
	
	FILE * fp2 = popen(str_cmd, "r");
	fgets(buffer2, sizeof(buffer2), fp2);
	pclose(fp2);
	
	//---------------------------
	printf("[pid=%d=0x%X] cpu mem usage: %s; %s\n", p, p, VmRSS, buffer2);

	return ret;
}


int print_date_time(const char *str)
{
	char str_time[80];
	struct tm newtime;
	time_t long_time = time(NULL);
	struct tm * pnewtime = localtime(&long_time);
	strftime(str_time, 80, "[%Y-%m-%d %H:%M:%S]", pnewtime);
	
	if(str)
	{
		printf("%s %s\n", str_time, str);
	}else
	{
		printf("%s\n", str_time);
	}

	return 0;
}


pid_t gettid(void)
{
	return syscall(SYS_gettid);
}


unsigned long get_current_thread_id()
{
//	pid_t id = gettid(); //getpid();

	pthread_t id = (pthread_t) pthread_self();

	return (unsigned long)id;
}


int create_nested_dir(const char *dir) //创建嵌套目录
{
	char dir1[600] = { 0 };
	char dir2[600] = { 0 };
	int flag = 0;
	int bRet = 0;

	strcpy(dir1, dir);

	int len = strlen(dir1);

	if (dir1[len - 1] != '/')
	{
		dir1[len] = '/';
		dir1[len + 1] = '\0';
		len++;
	}

	for (int i = 0; i < len; ++i)
	{
		if (dir1[i] == '/')
		{
			if (flag == 1)
			{
				bRet = access(dir2, F_OK);
				if (bRet != 0) //文件不存在
				{
					bRet = mkdir(dir2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
					if (bRet != 0)
					{
						printf("%s: Cannot create dir '%s'\n", __FUNCTION__, dir2);
						return -1;
					}
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

#endif // #if !defined(_WIN32) && !defined(_WIN64)
