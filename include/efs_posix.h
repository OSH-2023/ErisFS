#ifndef __EFS_POSIX_H__
#define __EFS_POSIX_H__

#include "headers.h"

typedef int ssize_t;

int efs_open(const char *file, int flags, ...);

int close(int fd);

ssize_t read(int fd, void *buf, size_t len);

ssize_t write(int fd, const void *buf, size_t len);

int fstat_(int fd, struct stat *buf);

int creat(const char *path, mode_t mode);

int statfs(const char *path, struct statfs *buf);

int stat(const char *file, struct stat *buf);

int unlink(const char *pathname);

int rename(const char *old_file, const char *new_file);

off_t lseek(int fd, off_t offset, int whence);

int creat(const char *path, mode_t mode);

int ftruncate(int fd, off_t length);

#endif