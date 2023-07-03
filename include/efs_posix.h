#ifndef __EFS_POSIX_H__
#define __EFS_POSIX_H__


int efs_open(const char *file, int flags, ...);

int close(int fd);

typedef int ssize_t;

ssize_t read(int fd, void *buf, size_t len);

ssize_t write(int fd, const void *buf, size_t len);

int fstat_(int fd, struct stat *buf);

int creat(const char *path, mode_t mode);


#endif