#ifndef __LOG_FILE_H__
#define __LOG_FILE_H__

#include <stdio.h>
#include "thread.h"


#if defined(_WIN32) || defined(_WIN64)

#else
//	#define __FUNCTION__     __PRETTY_FUNCTION__
#endif


class CLogFile
{
public:
	static int      m_ref_cnt; //引用计数
	static FILE *   m_fp_log; //日志文件句柄
	static CMutex   m_mutex;

	bool            m_is_print_to_file; //是否将信息打印到磁盘文件。[默认是]
	bool            m_is_print_to_stdio; //是否将信息打印到标准输出。[默认是]

	char            m_log_path[600];

public:
	CLogFile();
	CLogFile(char *log_path);
	~CLogFile();

	int createLogFile();
	int closeLogFile();
	int writeLogFile(const char *pszFormat, ...);
	int getTimeStr(char *timeStr);
	int getExeDirPath(char *dir_path, int size);
	bool isFolderExist(const char *dir); //文件夹路径是否存在
	int createDirectory(const char *pathName);
	int readConfigFile(char *filename);
};

#endif //__LOG_FILE_H__
