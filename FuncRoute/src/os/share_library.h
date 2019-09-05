#ifndef __SHARE_LIBRARY_H__
#define __SHARE_LIBRARY_H__

#include <stdint.h>
#include <vector>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
	#define SHARE_LIB_PREFIX    ""
	#define SHARE_LIB_SUFFIX    ".dll"
#else
	#define SHARE_LIB_PREFIX    "lib"
	#define SHARE_LIB_SUFFIX    ".so"
#endif


typedef void * share_library_handle;
typedef void (* func_pointer)(void);

share_library_handle share_library_load(const char *file_name);
func_pointer share_library_get_func_addr(share_library_handle handle, const char *func_name);
int share_library_free(share_library_handle handle);

int get_exe_dir_path(char *dir_path, int size);
int get_children_dir_name(char *parent_dir_path, std::vector<std::string> &children_dir_name);
int get_dir_files(const char *dir_path, std::vector<std::string> &files);
int get_nested_dir_files(const char *dir_path, std::vector<std::string> &files);
int set_dll_directory(const char *dir_path);
bool is_file_exist(const char *file_path);

int print_mem_usage();
int print_date_time(const char *str);

unsigned long get_current_thread_id();

int create_nested_dir(const char *dir); //´´½¨Ç¶Ì×Ä¿Â¼


#endif //__SHARE_LIBRARY_H__
