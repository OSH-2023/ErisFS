#include "../include/efs.h"
#include "../include/efs_file.h"
#include "../include/efs_private.h"
#include <unistd.h>

#define EFS_FNODE_HASH_NR 128

struct efs_vnode_mgr
{
    struct mutex lock;  // need solving 
    list_t head[EFS_FNODE_HASH_NR];
};

static struct efs_vnode_mgr efs_fm;

void efs_fm_lock(void)
{
    mutex_take(&efs_fm.lock, WAITING_FOREVER);  // need solving 
}

void efs_fm_unlock(void)
{
    mutex_release(&efs_fm.lock);
}

void efs_vnode_mgr_init(void)
{
    int i = 0;

    mutex_init(&efs_fm.lock, "efs_mgr", IPC_FLAG_PRIO); // need solving 
    for (i = 0; i < EFS_FNODE_HASH_NR; i++)
    {
        list_init(&efs_fm.head[i]);
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

static struct efs_vnode * efs_vnode_find(const char * path, list_t ** hash_head)
{
    struct efs_vnode *vnode = NULL;
    int hash = bkdr_hash(path);
    list_t * hh;

    hh = efs_fm.head[hash].next;

    if (hash_head)
    {
        *hash_head = &efs_fm.head[hash];
    }

    while (hh != &efs_fm.head[hash])
    {
        vnode = container_of(hh, struct efs_vnode, list);   // need solving 
        if (strcmp(path, vnode->fullpath) == 0)
        {
            /* found */
            return vnode;
        }
        hh = hh->next;
    }
    return NULL;
}

/**
 * @addtogroup FileApi
 * @{
 */

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
    struct efs_vnode *vnode = NULL;
    int ret = 0;

    fullpath = efs_normalize_path(NULL, pathname);

    efs_fm_lock();
    vnode = efs_vnode_find(fullpath, NULL);
    if (vnode)
    {
        ret = 1;
    }
    efs_fm_unlock();

    free(fullpath);
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
    struct efs_filesystem * fs;
    char * fullpath;
    int result;
    struct efs_vnode * vnode = NULL;
    list_t * hash_head;

    /* parameter check */
    if (fd == NULL)
        return -pdFREERTOS_ERRNO_EINVAL;

    /* make sure we have an absolute path */
    fullpath = efs_normalize_path(NULL, path);
    if (fullpath == NULL)
    {
        return -pdFREERTOS_ERRNO_ENOMEM;
    }

    LOG_D("open file:%s", fullpath);

    efs_fm_lock();
    /* vnode find */
    vnode = efs_vnode_find(fullpath, &hash_head);
    if (vnode)
    {
        vnode->ref_count++;
        fd->pos   = 0;
        fd->vnode = vnode;
        efs_fm_unlock();
        free(fullpath); /* release path */
    }
    else
    {
        /* find filesystem */
        fs = efs_filesystem_lookup(fullpath);
        if (fs == NULL)
        {
            efs_fm_unlock();
            free(fullpath); /* release path */
            return -pdFREERTOS_ERRNO_ENOENT;
        }

        vnode = calloc(1, sizeof(struct efs_vnode));
        if (!vnode)
        {
            efs_fm_unlock();
            free(fullpath); /* release path */
            return -pdFREERTOS_ERRNO_ENOMEM;
        }
        vnode->ref_count = 1;

        LOG_D("open in filesystem:%s", fs->ops->name);
        vnode->fs    = fs;             /* set file system */
        vnode->fops  = fs->ops->fops;  /* set file ops */

        /* initialize the fd item */
        vnode->type  = FT_REGULAR;
        vnode->flags = 0;

        if (!(fs->ops->flags & EFS_FS_FLAG_FULLPATH))
        {
            if (efs_subdir(fs->path, fullpath) == NULL)
                vnode->path = strdup("/");
            else
                vnode->path = strdup(efs_subdir(fs->path, fullpath));
            LOG_D("Actual file path: %s", vnode->path);
        }
        else
        {
            vnode->path = fullpath;
        }
        vnode->fullpath = fullpath;

        /* specific file system open routine */
        if (vnode->fops->open == NULL)
        {
            efs_fm_unlock();
            /* clear fd */
            if (vnode->path != vnode->fullpath)
            {
                free(vnode->fullpath);
            }
            free(vnode->path);
            free(vnode);

            return -pdFREERTOS_ERRNO_EINTR;
        }

        fd->pos   = 0;
        fd->vnode = vnode;

        /* insert vnode to hash */
        list_inseafter(hash_head, &vnode->list);
    }

    fd->flags = flags;

    if ((result = vnode->fops->open(fd)) < 0)
    {
        vnode->ref_count--;
        if (vnode->ref_count == 0)
        {
            /* remove from hash */
            list_remove(&vnode->list);
            /* clear fd */
            if (vnode->path != vnode->fullpath)
            {
                free(vnode->fullpath);
            }
            free(vnode->path);
            fd->vnode = NULL;
            free(vnode);
        }

        efs_fm_unlock();
        LOG_D("%s open failed", fullpath);

        return result;
    }

    fd->flags |= EFS_F_OPEN;    // need solving 
    if (flags & O_DIRECTORY)
    {
        fd->vnode->type = FT_DIRECTORY; // need solving 
        fd->flags |= EFS_F_DIRECTORY;   // need solving 
    }
    efs_fm_unlock();

    LOG_D("open successful");
    return 0;
}

/**
 * this function will close a file descriptor.
 *
 * @param fd the file descriptor to be closed.
 *
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
            list_remove(&vnode->list);
            fd->vnode = NULL;

            if (vnode->path != vnode->fullpath)
            {
                free(vnode->fullpath);
            }
            free(vnode->path);
            free(vnode);
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
 *
 * @param fd the directory descriptor.
 * @param dirp the dirent buffer to save result.
 * @param nbytes the available room in the buffer.
 *
 * @return the read dirent, others on failed.
 */
int efs_file_getdents(struct efs_file * fd, struct dirent * dirp, size_t nbytes)
{
    /* parameter check */
    if (fd == NULL)
    {
        return -pdFREERTOS_ERRNO_EINVAL;   
    }

    if (fd->vnode->type != FT_DIRECTORY)    // need solving 
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    if (fd->vnode->fops->getdents != NULL)
    {
        return fd->vnode->fops->getdents(fd, dirp, nbytes);
    }

    return -pdFREERTOS_ERRNO_EINTR;
}

/**
 * this function will unlink (remove) a specified path file from file system.
 *
 * @param path the specified path file to be unlinked.
 *
 * @return 0 on successful, -1 on failed.
 */
int efs_file_unlink(const char * path)
{
    int result;
    char * fullpath;
    struct efs_filesystem * fs;

    /* Make sure we have an absolute path */
    fullpath = efs_normalize_path(NULL, path);
    if (fullpath == NULL)
    {
        return -pdFREERTOS_ERRNO_EINVAL;
    }

    /* Check whether file is already open */
    if (efs_file_is_open(fullpath))
    {
        result = -pdFREERTOS_ERRNO_EBUSY;
        goto __exit;
    }

    /* get filesystem */
    if ((fs = efs_filesystem_lookup(fullpath)) == NULL)
    {
        result = -pdFREERTOS_ERRNO_ENOENT;
        goto __exit;
    }

    if (fs->ops->unlink != NULL)
    {
        if (!(fs->ops->flags & EFS_FS_FLAG_FULLPATH))
        {
            if (efs_subdir(fs->path, fullpath) == NULL)
                result = fs->ops->unlink(fs, "/");
            else
                result = fs->ops->unlink(fs, efs_subdir(fs->path, fullpath));
        }
        else
            result = fs->ops->unlink(fs, fullpath);
    }
    else result = -pdFREERTOS_ERRNO_EINTR;

__exit:
    free(fullpath);
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
 *
 * @param fd the file descriptor.
 *
 * @return 0 on successful, -1 on failed.
 */
int efs_file_flush(struct efs_file * fd)
{
    if (fd == NULL)
        return -pdFREERTOS_ERRNO_EINVAL;

    if (fd->vnode->fops->flush == NULL)
        return -pdFREERTOS_ERRNO_EINTR;

    return fd->vnode->fops->flush(fd);
}

/**
 * this function will seek the offset for specified file descriptor.
 *
 * @param fd the file descriptor.
 * @param offset the offset to be sought.
 *
 * @return the current position after seek.
 */
int efs_file_lseek(struct efs_file * fd, off_t offset)
{
    int result;

    if (fd == NULL)
        return -pdFREERTOS_ERRNO_EINVAL;

    if (fd->vnode->fops->lseek == NULL)
        return -pdFREERTOS_ERRNO_EINTR;

    result = fd->vnode->fops->lseek(fd, offset);

    /* update current position */
    if (result >= 0)
        fd->pos = result;

    return result;
}

/**
 * this function will get file information.
 *
 * @param path the file path.
 * @param buf the data buffer to save stat description.
 *
 * @return 0 on successful, -1 on failed.
 */
int efs_file_stat(const char * path, struct stat * buf)
{
    int result;
    char * fullpath;
    struct efs_filesystem *fs;

    fullpath = efs_normalize_path(NULL, path);
    if (fullpath == NULL)
    {
        return -1;
    }

    if ((fs = efs_filesystem_lookup(fullpath)) == NULL)
    {
        LOG_E("can't find mounted filesystem on this path:%s", fullpath);
        free(fullpath);

        return -pdFREERTOS_ERRNO_ENOENT;
    }

    if ((fullpath[0] == '/' && fullpath[1] == '\0') ||
        (efs_subdir(fs->path, fullpath) == NULL))
    {
        /* it's the root directory */
        buf->st_dev   = 0;

        buf->st_mode  = S_IRUSR | S_IRGRP | S_IROTH |
                        S_IWUSR | S_IWGRP | S_IWOTH;
        buf->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;

        buf->st_size    = 0;
        buf->st_mtime   = 0;

        /* release full path */
        free(fullpath);

        return pdFREERTOS_ERRNO_NONE;
    }
    else
    {
        if (fs->ops->stat == NULL)
        {
            free(fullpath);
            LOG_E("the filesystem didn't implement this function");

            return -pdFREERTOS_ERRNO_EINTR;
        }

        /* get the real file path and get file stat */
        if (fs->ops->flags & EFS_FS_FLAG_FULLPATH)  // need solving 
            result = fs->ops->stat(fs, fullpath, buf);
        else
            result = fs->ops->stat(fs, efs_subdir(fs->path, fullpath), buf);
    }

    free(fullpath);

    return result;
}

/**
 * this function will rename an old path name to a new path name.
 *
 * @param oldpath the old path name.
 * @param newpath the new path name.
 *
 * @return 0 on successful, -1 on failed.
 */
int efs_file_rename(const char * oldpath, const char * newpath)
{
    int result = pdFREERTOS_ERRNO_NONE;
    struct efs_filesystem * olefs = NULL, * newfs = NULL;
    char * oldfullpath = NULL, * newfullpath = NULL;

    newfullpath = NULL;
    oldfullpath = NULL;

    oldfullpath = efs_normalize_path(NULL, oldpath);
    if (oldfullpath == NULL)
    {
        result = -pdFREERTOS_ERRNO_ENOENT;
        goto __exit;
    }

    if (efs_file_is_open((const char *) oldfullpath))
    {
        result = -pdFREERTOS_ERRNO_EBUSY;
        goto __exit;
    }

    newfullpath = efs_normalize_path(NULL, newpath);
    if (newfullpath == NULL)
    {
        result = -pdFREERTOS_ERRNO_ENOENT;
        goto __exit;
    }

    olefs = efs_filesystem_lookup(oldfullpath);
    newfs = efs_filesystem_lookup(newfullpath);

    if (olefs == newfs)
    {
        if (olefs->ops->rename == NULL)
        {
            result = -pdFREERTOS_ERRNO_EINTR;
        }
        else
        {
            if (olefs->ops->flags & EFS_FS_FLAG_FULLPATH)   // need solving 
                result = olefs->ops->rename(olefs, oldfullpath, newfullpath);
            else
                /* use sub directory to rename in file system */
                result = olefs->ops->rename(olefs,
                                            efs_subdir(olefs->path, oldfullpath),
                                            efs_subdir(newfs->path, newfullpath));
        }
    }
    else
    {
        result = -pdFREERTOS_ERRNO_EXDEV;
    }

__exit:
    if (oldfullpath)
    {
        free(oldfullpath);
    }
    if (newfullpath)
    {
        free(newfullpath);
    }

    /* not at same file system, return pdFREERTOS_ERRNO_EXDEV */
    return result;
}