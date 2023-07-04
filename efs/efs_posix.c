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
        printf("[efs_posix.c]failed to open a file in efs_posix_fd_new!\n");

        return -1;
    }
    printf("fd_new: %d\n", fd);
    d = fd_get(fd);
    result = efs_file_open(d, file, flags);
    if (result < 0)
    {
        /* release the ref-count of fd */
        fd_release(fd);

        printf("[efs_posix.c]failed to open a file in efs_posix_efs_file_open!\n");

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
        printf("[efs_posix.c]failed to get the file in efs_posix_close_fd_get!\n");

        return -1;
    }

    result = efs_file_close(d);

    if (result < 0)
    {
        printf("[efs_posix.c]failed to close the file in efs_posix_close_efs_file_close!\n");

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
        printf("[efs_posix.c]failed to get the file in efs_posix_fd_get!\n");

        return -1;
    }

    result = efs_file_read(d, buf, len);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to close the file in efs_posix_efs_file_read!\n");

        return -1;
    }

    return result;
}

ssize_t write(int fd, const void *buf, size_t len)
{
    int result;
    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        printf("[efs_posix.c]failed to get the file in efs_posix_write_fd_get!\n");
        return -1;
    }

    result = efs_file_write(d, buf, len);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to close the file in efs_posix_efs_file_write!\n");
        return -1;
    }

    return result;
}

int fstat_(int fd, struct stat *buf)
{
    if (buf == NULL)
    {
        printf("[efs_posix.c] fstat: no enough buf!\n");
        return -1;
    }

    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        printf("[efs_posix.c] fstat: failed to get the file!\n");
        return -1;
    }

    return stat(d->vnode->fullpath, buf);
}

off_t lseek(int fd, off_t offset, int whence)
{
    int result;
    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        printf("[efs_posix.c]failed to open a file in efs_posix_lseek!\n");

        return -1;
    }

    switch (whence)
    {
    case SEEK_SET:
        break;

    case SEEK_CUR:
        offset += d->pos;
        break;

    case SEEK_END:
        offset += d->vnode->size;
        break;

    default:
        printf("[efs_posix.c]no valid whence in efs_posix_lseek!\n");

        return -1;
    }

    if (offset < 0)
    {
        printf("[efs_posix.c]no valid offset in efs_posix_lseek!\n");

        return -1;
    }
    result = efs_file_lseek(d, offset);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to seek the result in efs_posix_lseek!\n");

        return -1;
    }

    return offset;
}

int rename(const char *old_file, const char *new_file)
{
    int result;

    result = efs_file_rename(old_file, new_file);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to get the result in efs_posix_rename!\n");

        return -1;
    }

    return 0;
}

int unlink(const char *pathname)
{
    int result;

    result = efs_file_unlink(pathname);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to get the result in efs_posix_unlink!\n");

        return -1;
    }

    return 0;
}

int stat(const char *file, struct stat *buf)
{
    int result;

    result = efs_file_stat(file, buf);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to get the result in efs_posix_stat!\n");

        return -1;
    }

    return result;
}

int statfs(const char *path, struct statfs *buf)
{
    int result;

    result = efs_statfs(path, buf);
    if (result < 0)
    {
        printf("[efs_posix.c]failed to get the result in efs_posix_statfs!\n");

        return -1;
    }

    return result;
}

int encrypt(char * file_path)
{
    int fd0, fd1, i = 0;
    unsigned long int bytes;
    struct efs_file * d0;

    fd0 = efs_open(file_path, O_RDWR, 0);
    // no such file
    if(fd0 == -1)
    {
        return -1;
    }
    d0 = fd_get(fd0);
    bytes = d0->vnode->size;
    char buffer0[bytes], buffer1[bytes], ch;

    fd1 = efs_open("__temp.txt", O_CREAT, 0);
    // cant create file
    if(fd1 == -1)
    {
        return -1;
    }

    read(fd0, buffer0, bytes);
    ch = buffer0[0];

    while(ch != EOF)
    {
        ch = ch + 100;
        buffer1[i] = ch;
        ch = buffer0[++i];
    }

    write(fd1, buffer1, bytes);

    close(fd0);
    close(fd1);

    // fd0 = fopen(file_path, "w");
    fd0 = efs_open(file_path, O_RDWR, 0);

    if(fd0 == -1)
    {
        return -1;
    }

    fd1 = efs_open("__temp.txt", O_RDWR, 0);
    if(fd1 == -1)
    {
        return -1;
    }

    read(fd1, buffer1, bytes);
    i = 0;

    while(ch != EOF)
    {
        ch = buffer1[i];
        buffer0[i++] = ch;
    }

    write(fd0, buffer0, bytes);

    close(fd0);
    close(fd1);

    unlink("__temp.txt");
    printf("\nFile %s Encrypted Successfully!", file_path);
    return 0;
}

int decrypt(char * file_path)
{
    int fd0, fd1, i = 0;
    unsigned long int bytes;
    struct efs_file * d0;

    fd0 = efs_open(file_path, O_RDWR, 0);
    // no such file
    if(fd0 == -1)
    {
        return -1;
    }

    d0 = fd_get(fd0);
    bytes = d0->vnode->size;
    char buffer0[bytes], buffer1[bytes], ch;

    fd1 = efs_open("__temp.txt", O_CREAT, 0);
    // cant create file
    if(fd1 == -1)
    {
        return -1;
    }

    read(fd0, buffer0, bytes);
    ch = buffer0[0];

    while(ch != EOF)
    {
        ch = ch - 100;
        buffer1[i] = ch;
        ch = buffer0[++i];
    }

    write(fd1, buffer1, bytes);

    close(fd0);
    close(fd1);

    // fd0 = fopen(file_path, "w");
    fd0 = efs_open(file_path, O_RDWR, 0);

    if(fd0 == -1)
    {
        return -1;
    }

    fd1 = efs_open("__temp.txt", O_RDWR, 0);
    if(fd1 == -1)
    {
        return -1;
    }

    read(fd1, buffer1, bytes);
    i = 0;

    while(ch != EOF)
    {
        ch = buffer1[i];
        buffer0[i++] = ch;
    }

    write(fd0, buffer0, bytes);

    close(fd0);
    close(fd1);

    unlink("__temp.txt");
    printf("\nFile %s Decrypted Successfully!", file_path);
    return 0;
}

int creat(const char *path, mode_t mode)
{
    return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int ftruncate(int fd, off_t length)
{
    int result;
    struct dfs_fd *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        printf("[efs_posix.c] failed to get the file in efs_posix_ftruncate!\n");
        return -EBADF;
    }

    if (length < 0)
    {
        printf("[efs_posix.c] length cannot be negtive in efs_posix_ftruncate!\n");
        return -EINVAL;
    }
    result = dfs_file_ftruncate(d, length);
    if (result < 0)
    {
        return result;
    }

    return 0;
}