#include "headers.h"

#define EFS_FNODE_HASH_NR 128

struct efs_vnode_mgr
{
    SemaphoreHandle_t mutex;
    eris_list_t head[EFS_FNODE_HASH_NR];
};

static struct efs_vnode_mgr efs_fm;

void efs_fm_lock(void) 
{
    xSemaphoreTake( efs_fm.mutex, portMAX_DELAY);
}

void efs_fm_unlock(void) 
{
    xSemaphoreGive( efs_fm.mutex);
}

void efs_vnode_mgr_init(void)
{
    int i = 0; 
    efs_fm.mutex = xSemaphoreCreateMutex();
    for (i = 0; i < EFS_FNODE_HASH_NR; i++)
    {
        eris_list_init(&efs_fm.head[i]);
    }
}

/* BKDR Hash Function */
static unsigned int bkdr_hash(const char * str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (* str)
    {
        hash = hash * seed + (*str++);
    }

    return (hash % EFS_FNODE_HASH_NR);
}

static struct efs_vnode * efs_vnode_find(const char * path, eris_list_t ** hash_head)
{
    struct efs_vnode * vnode = NULL;
    int hash = bkdr_hash(path);         // get hash value
    eris_list_t * hh;   

    hh = efs_fm.head[hash].next;

    if (hash_head)
    {
        * hash_head = &efs_fm.head[hash];
    }

    while (hh != &efs_fm.head[hash])
    {
        vnode = container_of(hh, struct efs_vnode, list);  
        //printf("[efs_file.c] efs_file_open: find vnode: %s\r\n", vnode->fullpath);
        if (eris_strcmp(path, vnode->fullpath) == 0)
        {
            /* found */
            return vnode;
        }
        hh = hh->next;
    }
    //printf("[efs_file.c] efs_vnode_find: not found vnode: %s\r\n", path);
    return NULL;
}

/**
 * This function will return whether this file has been opend.
 *
 * @param pathname the file path name.
 *
 * @return 0 on file has been open successfully, -1 on open failed.
 */
int efs_file_is_open(const char * pathname)
{
    char * fullpath = NULL;
    struct efs_vnode * vnode = NULL;
    int ret = 0;

    fullpath = efs_normalize_path(NULL, pathname);
    //printf("[efs_file.c] efs_file_is_open: fullpath: %s\r\n", fullpath);

    efs_fm_lock();
    vnode = efs_vnode_find(fullpath, NULL);
    if (vnode)
    {
        ret = 1;
    }
    //printf("[efs_file.c] efs_file_is_open: ret: %d\r\n", ret);
    efs_fm_unlock();

    vPortFree(fullpath);
    return ret;
}


/**
 * this function will open a file which specified by path with specified flags.
 *
 * @param fd the file descriptor pointer to return the corresponding result.
 * @param path the specified file path.
 * @param flags the flags for open operator.
 *
 * @return 0 on successful, -1 on failed.
 */
int efs_file_open(struct efs_file * fd, const char * path, int flags)
{
    printf("[efs_file.c] efs_file_open: path: %s\r\n", path);
    struct efs_filesystem * fs;
    char * fullpath;
    int result;
    struct efs_vnode * vnode = NULL;
    eris_list_t * hash_head;

    /* parameter check */
    if (fd == NULL)
    {
        printf("1\r\r\n");
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    /* make sure we have an absolute path */
    fullpath = efs_normalize_path(NULL, path);
    printf("[efs_file.c] efs_file_open: fullpath: %s\r\n", fullpath);
    if (fullpath == NULL)
    {
         printf("2\r\r\n");
        return -pdFREERTOS_ERRNO_ENOMEM;
    }

    printf("open file: %s \r\n", fullpath);

    efs_fm_lock();
    /* vnode find */
    vnode = efs_vnode_find(fullpath, &hash_head);
    if (vnode)
    {
        //printf("[efs_file.c] efs_file_open: vnode exist");
        vnode->ref_count++;
        fd->pos   = 0;
        fd->vnode = vnode;
        efs_fm_unlock();
        vPortFree(fullpath); /* release path */
    }
    else
    {
        //printf("[efs_file.c] efs_file_open: vnode not exist, creating vnode\r\n");
        /* find filesystem */
        fs = efs_filesystem_lookup(fullpath);

        // not taken
        if (fs == NULL)
        {
            efs_fm_unlock();
            vPortFree(fullpath); /* release path */
            printf("[efs_file.c] efs_file_open: can't find mounted filesystem on this path: %s\r\n", fullpath);
            //printf("3\r\r\n");
            return -pdFREERTOS_ERRNO_ENOENT;
        }

        //vnode = pvPortCalloc(1, sizeof(struct efs_vnode));
        vnode = pvPortMalloc(sizeof(struct efs_vnode));

        // not taken
        if (!vnode)
        {
            efs_fm_unlock();
            vPortFree(fullpath); /* release path */
            printf("[efs_file.c] efs_file_open: can't malloc vnode!\r\n");
            //printf("4\r\r\n");
            return -pdFREERTOS_ERRNO_ENOMEM;
        }
        vnode->ref_count = 1;
        //printf("[efs_file.c] efs_file_open: open in filesystem: %s\r\n", fs->ops->name);
        vnode->fs    = fs;             /* set file system */
        vnode->fops  = fs->ops->fops;  /* set file ops */

        /* initialize the fd item */
        vnode->type  = FT_REGULAR; 
        vnode->flags = 0;
        if (!(fs->ops->flags & EFS_FS_FLAG_FULLPATH))
        {
            if (efs_subdir(fs->path, fullpath) == NULL)
            {
                vnode->path = strdup_efs("/");
            }   
            else
            {
                vnode->path = strdup_efs(efs_subdir(fs->path, fullpath));
            }
            printf("[efs_file.c] efs_file_open: Actual file path: %s\r\n", vnode->path);
            //printf("[efs_fs.c] test\r\n");
        }
        else
        {
            vnode->path = strdup_efs(fullpath);
        }
        vnode->fullpath = strdup_efs(fullpath);
        /* specific file system open routine */
        if (vnode->fops->open == NULL)
        {
            efs_fm_unlock();
            /* clear fd */
            if (vnode->path != vnode->fullpath)
            {
                vPortFree(vnode->fullpath);
            }
            vPortFree(vnode->path);
            vPortFree(vnode);
            printf("[efs_file.c] efs_file_open: the filesystem didn't implement this open function");
            //printf("5\r\r\n");
            return -pdFREERTOS_ERRNO_EINTR;
        }

        fd->pos   = 0;
        fd->vnode = vnode;


        /* insert vnode to hash */
        eris_list_insert_after(hash_head, &vnode->list);
    }
    fd->flags = flags;

    // not taken
    if ((result = vnode->fops->open(fd)) < 0)
    {
        vnode->ref_count--;
        if (vnode->ref_count == 0)
        {
            /* remove from hash */
            eris_list_remove(&vnode->list);
            /* clear fd */
            if (vnode->path != vnode->fullpath)
            {
                vPortFree(vnode->fullpath);
            }
            vPortFree(vnode->path);
            fd->vnode = NULL;
            vPortFree(vnode);
        }
        //printf("[efs_file.c] test\r\n");
        efs_fm_unlock();
        printf("[efs_file.c] efs_file_open: %s open failed!\r\n", fullpath);

        return result;
    }

    fd->flags |= EFS_F_OPEN;    // need solving 
    if (flags & O_DIRECTORY)
    {
        printf("HI\r\n");
        fd->vnode->type = FT_DIRECTORY; // need solving 
        fd->flags |= EFS_F_DIRECTORY;   // need solving 
    }
    efs_fm_unlock();
    return 0;
}

/**
 * this function will close a file descriptor.
 * @return 0 on successful, -1 on failed.
 */
int efs_file_close(struct efs_file * fd)
{
    struct efs_vnode * vnode = NULL;
    int result = 0;

    if (fd == NULL)
    {
        return -pdFREERTOS_ERRNO_ENXIO;
    }

    if (fd->ref_count == 1)
    {
        efs_fm_lock();
        vnode = fd->vnode;
        if (vnode->ref_count <= 0)
        {
            efs_fm_unlock();
            return -pdFREERTOS_ERRNO_ENXIO;
        }
        if (vnode->fops->close != NULL)
        {
            result = vnode->fops->close(fd);
        }
        /* close fd error, return */
        if (result < 0)
        {
            efs_fm_unlock();
            return result;
        }
        if (vnode->ref_count == 1)
        {
            /* remove from hash */
            eris_list_remove(&vnode->list);
            fd->vnode = NULL;
            //if (vnode->path != vnode->fullpath)
            //{
            vPortFree(vnode->fullpath);
            //}
            vPortFree(vnode->path);
            vPortFree(vnode);
        }
        efs_fm_unlock();
    }

    return result;
}

/**
 * this function will read specified length data from a file descriptor to a
 * buffer.
 *
 * @param fd the file descriptor.
 * @param buf the buffer to save the read data.
 * @param len the length of data buffer to be read.
 *
 * @return the actual read data bytes or 0 on end of file or failed.
 */
int efs_file_read(struct efs_file * fd, void * buf, size_t len)
{
    int result = 0;

    if (fd == NULL)
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if (fd->vnode->fops->read == NULL)
    {
        return -pdFREERTOS_ERRNO_EINTR;
    }

    if ((result = fd->vnode->fops->read(fd, buf, len)) < 0)
    {
        fd->flags |= EFS_F_EOF; // need solving 
    }

    return result;
}

/**
 * this function will fetch directory entries from a directory descriptor.
 * @return the read dirent, others on failed.
 */
int efs_file_getdents(struct efs_file * fd, struct dirent * dirp, size_t nbytes)
{
    /* parameter check */
    if (fd == NULL)
    {
        printf("[efs_file.c] efs_file_getdents: fd is NULL\r\n");
        return -pdFREERTOS_ERRNO_EINVAL;   
    }

    if (fd->vnode->type != FT_DIRECTORY)    
    {
        printf("[efs_file.c] efs_file_getdents: fd is not a dir\r\n");
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if (fd->vnode->fops->getdents != NULL)
    {
        return fd->vnode->fops->getdents(fd, dirp, nbytes);
    }
    else 
    {
        printf("[efs_file.c] efs_file_getdents: getdents is NULL\r\n");
    }

    return -pdFREERTOS_ERRNO_EINTR;
}

/**
 * this function will unlink (remove) a specified path file from file system.
 * @return 0 on successful, -1 on failed.
 */
int efs_file_unlink(const char * path)
{
    int result;
    char * fullpath;
    struct efs_filesystem * fs;

    fullpath = efs_normalize_path(NULL, path);
    if (fullpath == NULL)
    {   printf("[efs_file.c] efs_file_rename: can't normalize path\r\n");
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if (efs_file_is_open(fullpath))
    {
        printf("[efs_file.c] efs_file_unlink: file is already open\r\n");
        vPortFree(fullpath);
        return -pdFREERTOS_ERRNO_EBUSY;
    }

    if ((fs = efs_filesystem_lookup(fullpath)) == NULL)
    {
        printf("[efs_file.c] efs_file_unlink: no corresponding filesystem\r\n");
        vPortFree(fullpath);
        return -pdFREERTOS_ERRNO_ENOENT;
    }

    if (fs->ops->unlink != NULL)
    {
        if (!(fs->ops->flags & EFS_FS_FLAG_FULLPATH))
        {
            //use sub dir
            if (efs_subdir(fs->path, fullpath) == NULL)
                // len(fullpath) = len(fs->path)
                result = fs->ops->unlink(fs, "/");
            else
                result = fs->ops->unlink(fs, efs_subdir(fs->path, fullpath));
        }
        else
            //use fullpath
            result = fs->ops->unlink(fs, fullpath);
    }
    else {
        result = -pdFREERTOS_ERRNO_EINTR;
        printf("[efs_file.c] efs_file_unlink: unlink is NULL\r\n");
    }

    vPortFree(fullpath);
    return result;
}

/**
 * this function will write some specified length data to file system.
 *
 * @param fd the file descriptor.
 * @param buf the data buffer to be written.
 * @param len the data buffer length
 *
 * @return the actual written data length.
 */
int efs_file_write(struct efs_file * fd, const void * buf, size_t len)
{
    if (fd == NULL)
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if (fd->vnode->fops->write == NULL)
    {
        return -pdFREERTOS_ERRNO_EINTR;
    }

    return fd->vnode->fops->write(fd, buf, len);
}

/**
 * this function will flush buffer on a file descriptor.
 * @return 0 on successful, -1 on failed.
int efs_file_flush(struct efs_file * fd)
{
    if (fd == NULL)
        return -pdFREERTOS_ERRNO_EINVAL;

    if (fd->vnode->fops->flush == NULL)
        return -pdFREERTOS_ERRNO_EINTR;

    return fd->vnode->fops->flush(fd);
}
 */

/**
 * this function will seek the offset for specified file descriptor.
 * @return the current position after seek.
 */
off_t efs_file_lseek(struct efs_file * fd, off_t offset)
{
    int result = -1;

    if (fd == NULL)
    {
        printf("[efs_file.c] efs_file_lseek: fd is NULL\r\n");
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if (fd->vnode->fops->lseek == NULL)
    {
        printf("[efs_file.c] efs_file_lseek: lseek is NULL\r\n");
        return -pdFREERTOS_ERRNO_EINTR;
    }

    result = fd->vnode->fops->lseek(fd, offset);

    if (result >= 0)
    {
        fd->pos = result;
    }
    return result;
}

/**
 * this function will get file information.
 * @return 0 on successful, -1 on failed.
 */
int efs_file_stat(const char *path, struct stat_efs *buf)
{
    int result;
    char * fullpath;
    struct efs_filesystem *fs;

    fullpath = efs_normalize_path(NULL, path);
    if (fullpath == NULL)
    {
        printf("[efs_file.c] efs_file_stat: can't normalize path\r\n");
        return -1;
    }

    if ((fs = efs_filesystem_lookup(fullpath)) == NULL)
    {
        printf("[efs_file.c] efs_file_stat: no corresponding filesystem\r\n");
        vPortFree(fullpath);
        return -pdFREERTOS_ERRNO_ENOENT;
    }

    if ((fullpath[0] == '/' && fullpath[1] == '\0') ||
        (efs_subdir(fs->path, fullpath) == NULL))
    {
        buf->st_dev   = 0;
        buf->st_mode  = S_IRUSR | S_IRGRP | S_IROTH |
                        S_IWUSR | S_IWGRP | S_IWOTH;
        buf->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        buf->st_size    = 0;

        vPortFree(fullpath);

        return pdFREERTOS_ERRNO_NONE;
    }
    else
    {
        if (fs->ops->stat == NULL)
        {
            vPortFree(fullpath);
            printf("[efs_file.c] efs_file_stat: stat is NULL\r\n");

            return -pdFREERTOS_ERRNO_EINTR;
        }

        if (fs->ops->flags & EFS_FS_FLAG_FULLPATH)
            result = fs->ops->stat(fs, fullpath, buf);
        else
            result = fs->ops->stat(fs, efs_subdir(fs->path, fullpath), buf);
    }

    vPortFree(fullpath);

    return result;
}


/**
 * this function will rename an old path name to a new path name.
 * @return 0 on successful, -1 on failed.
 */
int efs_file_rename(const char * old_path, const char * new_path)
{
    int result = -1;
    struct efs_filesystem * old_fs, * new_fs;
    char *old_full_path, *new_full_path;

    old_full_path = efs_normalize_path(NULL, old_path);
    if (old_full_path == NULL)
    {
        printf("[efs_file.c] efs_file_rename: can't normalize old_path\r\n");
        result = -pdFREERTOS_ERRNO_ENOENT;
        goto __exit;
    }

    if (efs_file_is_open((const char *)old_full_path))
    {
        printf("[efs_file.c] efs_file_rename: old_path is opened\r\n");
        result = -pdFREERTOS_ERRNO_EBUSY;
        goto __exit;
    }

    new_full_path = efs_normalize_path(NULL, new_path);
    if (new_full_path == NULL)
    {
        printf("[efs_file.c] efs_file_rename: can't normalize new_path\r\n");
        result = -pdFREERTOS_ERRNO_ENOENT;
        goto __exit;
    }

    old_fs = efs_filesystem_lookup(old_full_path);
    new_fs = efs_filesystem_lookup(new_full_path);

    if (old_fs == new_fs)
    {
        if (old_fs->ops->rename == NULL)
        {
            printf("[efs_file.c] efs_file_rename: rename is NULL\r\n");
            result = -pdFREERTOS_ERRNO_EINTR;
        }
        else
        {
            if (old_fs->ops->flags & EFS_FS_FLAG_FULLPATH)
                /* use fullpath */
                result = old_fs->ops->rename(old_fs, old_full_path, new_full_path);
            else
                /* use sub directory to fs->path */
                result = old_fs->ops->rename(old_fs,
                                            efs_subdir(old_fs->path, old_full_path),
                                            efs_subdir(old_fs->path, new_full_path));
        }
    }
    else
    {
        printf("[efs_file.c] efs_file_rename: old_path and new_path are not on the same filesystem\r\n");
        result = -pdFREERTOS_ERRNO_EXDEV;
    }

__exit:
    if (old_full_path)
    {
        vPortFree(old_full_path);
    }
    if (new_full_path)
    {
        vPortFree(new_full_path);
    }
    return result;
}

int efs_file_ftruncate(struct efs_file *fd, off_t length)
{
    int result;

    if (fd == NULL || fd->vnode->type != FT_REGULAR || length < 0) 
    {
        printf("[efs_file.c] efs_file_ftruncate: fd is NULL or not a regular file system fd\r\n");
        return -EINVAL;
    }

    if (fd->vnode->fops->ioctl == NULL)
        return -ENOSYS;

    result = fd->vnode->fops->ioctl(fd, EFS_FIOFTRUNCATE, (void*)&length); 
    // TO BE CHECKED

    if (result == 0)
        fd->vnode->size = length;

    return result;
}