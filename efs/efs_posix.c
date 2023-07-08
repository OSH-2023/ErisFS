#include "headers.h"

int efs_open(const char *file, int flags, ...)
{
    int fd, result;
    struct efs_file *d;
    /* allocate a fd */
    fd = fd_new();
    if (fd < 0)
    {
        // printf("[efs_posix.c]failed to open a file in efs_posix_fd_new!\r\n");
        return -1;
    }
    d = fd_get(fd);
    result = efs_file_open(d, file, flags);
    if (result < 0)
    {
        fd_release(fd);
        // printf("[efs_posix.c]failed to open a file in efs_posix_efs_file_open!\r\n");
 
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
        // printf("[efs_posix.c]failed to get the file in efs_posix_close_fd_get!\r\n");
        return -1;
    }

    result = efs_file_close(d);

    if (result < 0)
    {
        // printf("[efs_posix.c]failed to close the file in efs_posix_close_efs_file_close!\r\n");
        return -1;
    }

    fd_release(fd);
    return 0;
}

ssize_t read(int fd, void *buf, size_t len)
{
    int result;
    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_fd_get!\r\n");
        return -1;
    }

    result = efs_file_read(d, buf, len);
    if (result < 0)
    {
        // printf("[efs_posix.c]failed to close the file in efs_posix_efs_file_read!\r\n");
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
        // printf("[efs_posix.c]failed to get the file in efs_posix_write_fd_get!\r\n");
        return -1;
    }

    result = efs_file_write(d, buf, len);
    if (result < 0)
    {
        // printf("[efs_posix.c]failed to close the file in efs_posix_efs_file_write!\r\n");
        return -1;
    }

    return result;
}

int fstat_(int fd, struct stat_efs *buf)
{
    if (buf == NULL)
    {
        // printf("[efs_posix.c] fstat: no enough buf!\r\n");
        return -1;
    }

    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        // printf("[efs_posix.c] fstat: failed to get the file!\r\n");
        return -1;
    }

    return stat_efs(d->vnode->fullpath, buf);
}

off_t lseek(int fd, off_t offset, int whence)
{
    int result;
    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        // printf("[efs_posix.c]failed to open a file in efs_posix_lseek!\r\n");

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
        // printf("[efs_posix.c]no valid whence in efs_posix_lseek!\r\n");

        return -1;
    }

    if (offset < 0)
    {
        // printf("[efs_posix.c]no valid offset in efs_posix_lseek!\r\n");

        return -1;
    }
    result = efs_file_lseek(d, offset);
    if (result < 0)
    {
        // printf("[efs_posix.c]failed to seek the result in efs_posix_lseek!\r\n");

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
        // printf("[efs_posix.c]failed to get the result in efs_posix_rename!\r\n");

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
        // printf("[efs_posix.c]failed to get the result in efs_posix_unlink!\r\n");

        return -1;
    }

    return 0;
}

int stat_efs(const char *file, struct stat_efs *buf)
{
    int result;

    result = efs_file_stat(file, buf);
    if (result < 0)
    {
        // printf("[efs_posix.c]failed to get the result in efs_posix_stat!\r\n");

        return -1;
    }

    return result;
}

int statfs_efs(const char *path, struct statfs *buf)
{
    int result;

    result = efs_statfs(path, buf);
    if (result < 0)
    {
        // printf("[efs_posix.c]failed to get the result in efs_posix_statfs!\r\n");

        return -1;
    }

    return result;
}

int encryptSimple(const char * file_path, int key)
{
    int fd;
    unsigned long int bytes;
    struct efs_file * d;
    char* buffer;

    if(fd = efs_open(file_path, O_RDWR, 0) < 0)
    {
        return -1;
    }
    d = fd_get(fd);
    bytes = d->vnode->size;

    buffer = (char *)pvPortMalloc(bytes * sizeof(char));
    lseek(fd, 0, SEEK_SET);
    read(fd, buffer, bytes);

    for(int i = 0; i < bytes; i++)
    {
        if(buffer[i] != EFS_F_EOF)
            buffer[i] = (buffer[i] + key) % 128;
    }
    buffer[bytes - 1] = 0;

    lseek(fd, 0, SEEK_SET);
    write(fd, buffer, bytes);
    close(fd);

    vPortFree(buffer);

    printf("File %s Encrypted Successfully!\r\n", file_path);
    return 0;
}

int decryptSimple(const char * file_path, int key)
{
    int fd;
    unsigned long int bytes;
    struct efs_file * d;
    char * buffer;

    if(fd = efs_open(file_path, O_RDWR, 0) < 0)
    {
        return -1;
    }
    d = fd_get(fd);
    bytes = d->vnode->size;

    buffer = (char *)pvPortMalloc(bytes * sizeof(char));
    lseek(fd, 0, SEEK_SET);
    read(fd, buffer, bytes);

    for(int i = 0; i < bytes; i++)
    {
        if(buffer[i] != EFS_F_EOF)
            buffer[i] = (buffer[i] - key) % 128;
    }
    buffer[bytes - 1] = 0;

    lseek(fd, 0, SEEK_SET);
    write(fd, buffer, bytes);
    close(fd);

    vPortFree(buffer);

    printf("File %s Decrypted Successfully!\r\n", file_path);
    return 0;
}

int creat(const char *path, mode_t mode)
{
    return efs_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int ftruncate(int fd, off_t length)
{
    int result;
    struct efs_file *d;

    d = fd_get(fd);
    if (d == NULL)
    {
        // printf("[efs_posix.c] failed to get the file in efs_posix_ftruncate!\r\n");
        return -EBADF;
    }

    if (length < 0)
    {
        // printf("[efs_posix.c] length cannot be negative in efs_posix_ftruncate!\r\n");
        return -EINVAL;
    }
    result = efs_file_ftruncate(d, length);
    if (result < 0)
    {
        return result;
    }

    return 0;
}

int mkdir(const char *path, mode_t mode)
{
    int fd;
    struct efs_file *d;
    int result;

    fd = fd_new();
    if (fd == -1)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_mkdir!\r\n");

        return -1;
    }

    d = fd_get(fd);
    result = efs_file_open(d, path, O_DIRECTORY | O_CREAT);

    if (result < 0)
    {
        fd_release(fd);
        // printf("[efs_posix.c]failed to create directory in mkdir!\r\n");
        return -1;
    }

    efs_file_close(d);
    fd_release(fd);

    return 0;
}

int rmdir(const char *pathname)
{
    int result;

    result = efs_file_unlink(pathname);
    if (result < 0)
    {
        // printf("[efs_posix.c]failed to unlink in efs_posix_rmdir!\r\n");

        return -1;
    }

    return 0;
}

DIRE *opendir(const char *name)
{
    struct efs_file *d;
    int fd, result;
    DIRE *t;

    t = NULL;

    /* allocate a fd */
    fd = fd_new();
    if (fd == -1)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_opendir!\r\n");

        return NULL;
    }
    d = fd_get(fd);

    result = efs_file_open(d, name, O_RDONLY | O_DIRECTORY);
    if (result >= 0)
    {
        /* open successfully */
        t = (DIRE *) pvPortMalloc(sizeof(DIRE));//potential problem: pvportmalloc or pvportmalloc_efs
        if (t == NULL)
        {
            efs_file_close(d);
            fd_release(fd);
        }
        else
        {
            memset(t, 0, sizeof(DIRE));
            t->fd = fd;
        }

        return t;
    }

    /* open failed */
    fd_release(fd);
    // printf("[efs_posix.c]failed to release in efs_posix_opendir!\r\n");

    return NULL;
}

struct dirent *readdir(DIRE *d)
{
    int result;
    struct efs_file *fd;

    fd = fd_get(d->fd);
    if (fd == NULL)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_readdir!\r\n");
        return NULL;
    }

    if (d->num)
    {
        struct dirent *dirent_ptr;
        dirent_ptr = (struct dirent *)&d->buf[d->cur];
        d->cur += dirent_ptr->d_reclen;
    }

    if (!d->num || d->cur >= d->num)
    {
        /* get a new entry */
        result = efs_file_getdents(fd,
                                   (struct dirent *)d->buf,
                                    sizeof(d->buf) - 1);
        if (result <= 0)
        {
            // printf("[efs_posix.c]failed to getdents in efs_posix_readdir!\r\n");

            return NULL;
        }

        d->num = result;
        d->cur = 0; /* current entry index */
    }

    return (struct dirent *)(d->buf + d->cur);
}

long telldir(DIRE *d)
{
    struct efs_file *fd;
    long result;

    fd = fd_get(d->fd);
    if (fd == NULL)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_telldir!\r\n");

        return 0;
    }

    result = fd->pos - d->num + d->cur;

    return result;
}

void seekdir(DIRE *d, long offset)
{
    struct efs_file *fd;

    fd = fd_get(d->fd);
    if (fd == NULL)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_seekdir!\r\n");

        return ;
    }

    /* seek to the offset position of directory */
    if (efs_file_lseek(fd, offset) >= 0)
        d->num = d->cur = 0;
}

void rewinddir(DIRE *d)
{
    struct efs_file *fd;

    fd = fd_get(d->fd);
    if (fd == NULL)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_rewinddir!\r\n");

        return ;
    }

    /* seek to the beginning of directory */
    if (efs_file_lseek(fd, 0) >= 0)
        d->num = d->cur = 0;
}

int closedir(DIRE *d)
{
    int result;
    struct efs_file *fd;

    fd = fd_get(d->fd);
    if (fd == NULL)
    {
        // printf("[efs_posix.c]failed to get the file in efs_posix_rewinddir!\r\n");
        return -1;
    }

    result = efs_file_close(fd);
    fd_release(d->fd);

    vPortFree(d);//////?

    if (result < 0)
    {
        // printf("[efs_posix.c]failed to close in efs_posix_closedir!\r\n");
        return -1;
    }
    else
        return 0;
}