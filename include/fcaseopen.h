#pragma once

#include <stdio.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

extern FILE *fcaseopen(char const *path, char const *mode);

extern DIR* caseopendir(const char* dir);

extern int casestat(const char* pathname, struct stat *statbuf);

