#ifndef fcaseopen_h
#define fcaseopen_h

#include <stdio.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

extern FILE *fcaseopen(char const *path, char const *mode);

//extern GDir* g_dir_caseopen(char const *path);

//extern int g_casestat(const char *filename, GStatBuf *buf);

//extern gboolean g_casefile_test(const char* filename, GFileTest test);

extern DIR* caseopendir(const char* dir);

extern int casestat(const char* pathname, struct stat *statbuf);

#endif 
