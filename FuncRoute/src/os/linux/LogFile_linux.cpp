#if !defined(_WIN32) && !defined(_WIN64)

#include "../LogFile.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include "../../version.h"


volatile short g_log_file_ref_cnt = 0;
int CLogFile::m_ref_cnt = 0;
FILE * CLogFile::m_fp_log = NULL;
CMutex CLogFile::m_mutex;


CLogFile::CLogFile()
{
	memset(m_log_path, 0, sizeof(m_log_path));
	
	m_is_print_to_file = true;
	m_is_print_to_stdio = true;
	
	//-----------------------------------------------
	int size = 600; //MAX_PATH
	char exe_dir_path[600] = {0};

	int ret22 = getExeDirPath(exe_dir_path, size);
	if(ret22 == 0)
	{
		sprintf(exe_dir_path, "%s/video_decoder.cfg", exe_dir_path);
		int ret3 = readConfigFile(exe_dir_path); //给外部一次更改参数的机会
	}
	
	//-----------------------------------------------
//	if(m_ref_cnt == 0) //第一个类的实例负责创建日志文件
	if(__sync_fetch_and_add(&g_log_file_ref_cnt, 1) == 0)
	{
		int ret = createLogFile();
	}

	m_ref_cnt++;
}


CLogFile::CLogFile(char *log_path)
{
	sprintf(m_log_path, "%s", log_path);
	
	m_is_print_to_file = true;
	m_is_print_to_stdio = true;

	//-----------------------------------------------
	int size = 600; //MAX_PATH
	char exe_dir_path[600] = {0};

	int ret22 = getExeDirPath(exe_dir_path, size);
	if(ret22 == 0)
	{
		sprintf(exe_dir_path, "%s/video_decoder.cfg", exe_dir_path);
		int ret3 = readConfigFile(exe_dir_path); //给外部一次更改参数的机会
	}
	
	//-----------------------------------------------
//	if(m_ref_cnt == 0) //第一个类的实例负责创建日志文件
	if(__sync_fetch_and_add(&g_log_file_ref_cnt, 1) == 0)
	{
		int ret = createLogFile();
	}
	
	m_ref_cnt++;
}


CLogFile::~CLogFile()
{
	m_ref_cnt--;

//	if(m_ref_cnt == 0)
	if(__sync_sub_and_fetch(&g_log_file_ref_cnt, 1) == 0)
	{
		int ret = closeLogFile();
	}
}


int CLogFile::createLogFile()
{
	int ret = 0;
	int size = 600; //MAX_PATH
	char exe_dir_path[600] = {0};
	char log_dir_path[600] = {0};

	ret = getExeDirPath(exe_dir_path, size);
	if(ret != 0){return -21;}

	printf("[INFO] %s: exe dir path: %s\n", __FUNCTION__, exe_dir_path);

	sprintf(log_dir_path, "%s/log", exe_dir_path);

	bool isExist = isFolderExist(log_dir_path);
	if(isExist == true && m_is_print_to_file == true) //目录存在，才生成日志文件
	{
		char str_time[80];
		struct tm newtime;
		time_t long_time = time(NULL);
		struct tm * pnewtime = localtime(&long_time);
		strftime(str_time, 80, "%Y-%m-%d", pnewtime);
		
		sprintf(log_dir_path, "%s/video_decoder/%s", log_dir_path, str_time);
		if(isFolderExist(log_dir_path) == false)
		{
			ret = createDirectory(log_dir_path);
			if(ret != 0)
			{
				printf("Error: %s: can not create log dir path: %s\n", __FUNCTION__, log_dir_path);
				return -1;
			}
		}

		pid_t pid = getpid();
		
//		strftime(str_time, 80, "%H.%M.%S", &newtime);

		if(strlen(m_log_path) == 0)
		{
//			sprintf(m_log_path, "%s\\%s_pid%ld.log", log_dir_path, str_time, pid);
			sprintf(m_log_path, "%s/pid%ld.log", log_dir_path, pid);
		}
		printf("[INFO] %s: log file path: %s\n", __FUNCTION__, m_log_path);
		
		if(m_fp_log != NULL)
		{
			this->writeLogFile("Error: %s: the log file had been open before.\n", __FUNCTION__);
			return -2;
		}

		m_fp_log = fopen(m_log_path, "a"); //以附加的方式打开只写文件,若文件不存在，则会创建该文件。
		if(m_fp_log == NULL)
		{
			printf("Error: Cannot open file to write. [%s]\n", m_log_path);
			return -31; //打开文件失败
		}

		this->writeLogFile("[INFO] %s: open the log file successfully. %s\n", __FUNCTION__, m_log_path);
		this->writeLogFile("[INFO] %s: decoder version: %s\n", __FUNCTION__, VERSION_STR3(VERSION_STR));
	}

	return 0;
}


int CLogFile::closeLogFile()
{
	this->writeLogFile("[INFO] %s: close the log file.\n", __FUNCTION__);

	if(m_fp_log){fclose(m_fp_log); m_fp_log = NULL;}
	return 0;
}


int CLogFile::writeLogFile(const char *pszFormat, ...)
{
	CAutoMutex autoLock(m_mutex);

	int ret = 0;

	const int logSize = 2 * 1024;
	static char strLog[logSize];
	
	va_list args;
	va_start(args, pszFormat);
	vsnprintf(strLog, logSize - 1, pszFormat, args);
	va_end(args);

	strLog[logSize - 1] = '\0';
	
	if(m_fp_log)
	{
		char time_str[100] = {0};
		ret = getTimeStr(time_str);
//		fwrite(time_str, strlen(time_str), 1, m_fp_log);
//		fwrite(strLog, strlen(strLog), 1, m_fp_log);
		fprintf(m_fp_log, "%s%s", time_str, strLog);
		ret = fflush(m_fp_log);
	}
	
	if(m_is_print_to_stdio == true)
	{
		printf("%s", strLog);
	}
	
	return ret;
}


int CLogFile::getTimeStr(char *timeStr)
{
	int ret = 0;
	
	char str_time[80];
	struct tm newtime;
	time_t long_time = time(NULL);
	struct tm * pnewtime = localtime(&long_time);
	strftime(str_time, 80, "%Y-%m-%d %H:%M:%S", pnewtime);
	
//	pid_t tid = syscall(SYS_gettid);
	pthread_t tid = (pthread_t) pthread_self();

	sprintf(timeStr, "[%s] [TID 0x%08x] ", str_time, tid);

	return ret;
}


int CLogFile::getExeDirPath(char *dir_path, int size)
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

	return 0;
}


bool CLogFile::isFolderExist(const char *dir)
{
	int ret = access(dir, 0); //[0 == F_OK]  只判断是否存在
	
	return (ret == 0) ? true : false;
}


int CLogFile::createDirectory(const char *pathName)
{
	char path[600] = {0};
	char pathName2[600] = {0};

	sprintf(pathName2, "%s/", pathName);
	const char * pos = pathName2;

	while((pos = strchr(pos, '/')) != NULL)
	{
		memcpy(path, pathName2, pos - pathName2 + 1);
		pos++;
		if(access(path, 0) == 0)
		{
			continue;
		}else
		{
			int ret = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
			if(ret != 0)
			{
				return -1;
			}
		}
	}

	return 0;
}


/*
配置文件 video_decoder.cfg 格式（可以使用'//'注释掉一行）
//decoder_type=DECODER_TYPE_VIDEO_FFMPEG_CPU;
//decoder_thread_mode=DECODER_THREAD_MODE_SINGLE;
//number_of_packets=6000;
//number_of_pictures=0;
//is_demux_and_decoder_sync=false;
is_enable_decoder_plugin=true;
is_log_print_to_file=true;
is_log_print_to_stdio=true;
*/
int CLogFile::readConfigFile(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if(!fp){return -1;}

	char line[200];
	char decoder_type[] = "decoder_type=";
	char decoder_thread_mode[] = "decoder_thread_mode=";
	char number_of_packets[] = "number_of_packets=";
	char number_of_pictures[] = "number_of_pictures=";
	char is_demux_and_decoder_sync[] = "is_demux_and_decoder_sync=";
	char is_enable_decoder_plugin[] = "is_enable_decoder_plugin=";
	char is_log_print_to_file[] = "is_log_print_to_file=";
	char is_log_print_to_stdio[] = "is_log_print_to_stdio=";

	int decoder_type_len = strlen(decoder_type);
	int decoder_thread_mode_len = strlen(decoder_thread_mode);
	int number_of_packets_len = strlen(number_of_packets);
	int number_of_pictures_len = strlen(number_of_pictures);
	int is_demux_and_decoder_sync_len = strlen(is_demux_and_decoder_sync);
	int is_enable_decoder_plugin_len = strlen(is_enable_decoder_plugin);
	int is_log_print_to_file_len = strlen(is_log_print_to_file);
	int is_log_print_to_stdio_len = strlen(is_log_print_to_stdio);

	while(fgets(line, 200, fp) != NULL)
	{
		if(strncmp(line, decoder_type, decoder_type_len) == 0)
		{
			
		}else if(strncmp(line, decoder_thread_mode, decoder_thread_mode_len) == 0)
		{
			
		}else if(strncmp(line, number_of_packets, number_of_packets_len) == 0)
		{
			
		}else if(strncmp(line, number_of_pictures, number_of_pictures_len) == 0)
		{

		}else if(strncmp(line, is_demux_and_decoder_sync, is_demux_and_decoder_sync_len) == 0)
		{

		}else if(strncmp(line, is_enable_decoder_plugin, is_enable_decoder_plugin_len) == 0)
		{

		}else if(strncmp(line, is_log_print_to_file, is_log_print_to_file_len) == 0)
		{
			if(strncmp(line + is_log_print_to_file_len, "true;", strlen("true;")) == 0)
			{
				m_is_print_to_file = true;
			}else if(strncmp(line + is_log_print_to_file_len, "false;", strlen("false;")) == 0)
			{
				m_is_print_to_file = false;
			}
		}else if(strncmp(line, is_log_print_to_stdio, is_log_print_to_stdio_len) == 0)
		{
			if(strncmp(line + is_log_print_to_stdio_len, "true;", strlen("true;")) == 0)
			{
				m_is_print_to_stdio = true;
			}else if(strncmp(line + is_log_print_to_stdio_len, "false;", strlen("false;")) == 0)
			{
				m_is_print_to_stdio = false;
			}
		}
	}

	fclose(fp);

	return 0;
}

#endif // #if !defined(_WIN32) && !defined(_WIN64)
