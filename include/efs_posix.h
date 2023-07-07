#ifndef __EFS_POSIX_H__
#define __EFS_POSIX_H__

#include "headers.h"


#ifndef HAVE_DIR_STRUCTURE
#define HAVE_DIR_STRUCTURE
typedef struct
{
    int fd;                         /* directory file */
    char buf[512];
    int num;
    int cur;
}DIRE;
#endif
typedef unsigned short mode_t;

typedef int ssize_t;

int efs_open(const char *file, int flags, ...);

int close(int fd);

ssize_t read(int fd, void *buf, size_t len);

ssize_t write(int fd, const void *buf, size_t len);

int stat_efs(const char *file, struct stat_efs *buf);

int fstat_(int fd, struct stat_efs *buf);

//modes are not used
int creat(const char *path, mode_t mode);

int statfs_efs(const char *path, struct statfs *buf);

int unlink(const char *pathname);

int rename(const char *old_file, const char *new_file);

off_t lseek(int fd, off_t offset, int whence);

int ftruncate(int fd, off_t length);

int encryptSimple(const char *, int);

int decryptSimple(const char *, int);

int mkdir(const char *path, mode_t mode);

int rmdir(const char *pathname);

DIRE *opendir(const char *name);

struct dirent *readdir(DIRE *d);

long telldir(DIRE *d);

void seekdir(DIRE *d, long offset);

void rewinddir(DIRE *d);

int closedir(DIRE *d);



#endif