#include <stdio.h>
#include <efs_private.h> //rthread中对应一些表 待替换
#include <sys/errno.h>
#include <efs.h>
#include <efs_fs.h>
#include <efs_device.h>
#include <efs_file.h>

int efs_open(const char *file, int flags, ...)
{
    int fd, result;
    struct efs_file *d;
    /* allocate a fd */
    fd = fd_new();
    if (fd < 0)
    {
        printf("failed to open a file in efs_posix_fd_new!\n");

        return -1;
    }
    printf("fd_new: %d\n", fd);
    d = fd_get(fd);
    result = efs_file_open(d, file, flags);
    if (result < 0)
    {
        /* release the ref-count of fd */
        fd_release(fd);

        printf("failed to open a file in efs_posix_efs_file_open!\n");

        return -1;
    }

    return fd;
}

int close(int fd)
{
    int result;
    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        printf("failed to get the file in efs_posix_close_fd_get!\n");

        return -1;
    }

    result = efs_file_close(d);

    if (result < 0)
    {
        printf("failed to close the file in efs_posix_close_efs_file_close!\n");

        return -1;
    }

    fd_release(fd);

    return 0;
}

ssize_t read(int fd, void *buf, size_t len)
{
    int result;
    struct efs_file *d;

    /* get the fd */
    d = fd_get(fd);
    if (d == NULL)
    {
        printf("failed to get the file in efs_posix_fd_get!\n");

        return -1;
    }

    result = efs_file_read(d, buf, len);
    if (result < 0)
    {
        printf("failed to close the file in efs_posix_efs_file_read!\n");

        return -1;
    }

    return result;
}

ssize_t write(int fd, const void *buf, size_t len)
{
    int result;
    struct efs_file *d;

    /* get the fd */
    d = fd_get(fd);
    if (d == NULL)
    {
         printf("failed to get the file in efs_posix_write_fd_get!\n");

        return -1;
    }

    result = efs_file_write(d, buf, len);
    if (result < 0)
    {
        printf("failed to close the file in efs_posix_efs_file_write!\n");

        return -1;
    }

    return result;
}