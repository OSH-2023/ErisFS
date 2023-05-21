#include <efs_file.h>
//#include <dfs_private.h> rthread中对应一些表 待替换
#include <sys/errno.h>
#include <efs.h>
#include <efs_fs.h>
#include <efs_device.h>

int open(const char *file, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t len);
ssize_t write(int fd, const void *buf, size_t len);


