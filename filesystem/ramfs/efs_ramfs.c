#include "headers.h"

int efs_ramfs_mount(struct efs_filesystem *fs,
                    unsigned long          rwflag,
                    const void            *data)
{
    struct efs_ramfs *ramfs;

    if (data == NULL)
        return -EIO;

    ramfs = (struct efs_ramfs *)data;
    fs->data = ramfs;

    return Eris_EOK;
}

int efs_ramfs_unmount(struct efs_filesystem *fs)
{
    fs->data = NULL;

    return Eris_EOK;
}

int efs_ramfs_statfs(struct efs_filesystem *fs, struct statfs *buf)
{
    struct efs_ramfs *ramfs;

    ramfs = (struct efs_ramfs *)fs->data;
    Eris_ASSERT(ramfs != NULL);                                       //未修改 freertos
    //Eris_ASSERT(buf != NULL);                                     

    buf->f_bsize  = 512;    
    buf->f_blocks = ramfs->memheap.pool_size / 512;
    buf->f_bfree  = ramfs->memheap.xFreeBytesRemaining / 512;

    return Eris_EOK;
}

int efs_ramfs_ioctl(struct efs_file *file, int cmd, void *args)
{
    return -EIO;
}


int efs_ramfs_read(struct efs_file *file, void *buf, size_t count)
{
    size_t length;
    struct ramfs_dirent *dirent;

    dirent = (struct ramfs_dirent *)file->vnode->data;
    Eris_ASSERT(dirent != NULL);

    if (count < file->vnode->size - file->pos)
        length = count;
    else
        length = file->vnode->size - file->pos;

    if (length > 0)
    {
        eris_memcpy(buf, &(dirent->data[file->pos]), length);
    }

    file->pos += length;

    return length;
}

struct ramfs_dirent *efs_ramfs_lookup(struct efs_ramfs *ramfs,
                                      const char       *path,
                                      size_t        *size)
{
    const char *subpath;
    struct ramfs_dirent *dirent;

    subpath = path;
    while (*subpath == '/' && *subpath)
        subpath ++;
    if (! *subpath) /* is root directory */
    {
        *size = 0;

        return &(ramfs->root);
    }

    for (dirent = eris_list_entry(ramfs->root.list.next, struct ramfs_dirent, list);
         dirent != &(ramfs->root);
         dirent = eris_list_entry(dirent->list.next, struct ramfs_dirent, list))
    {
        //printf("DEBUG: dirent->name: %s\n", dirent->name);
        //printf("DEBUG: subpath: %s\n", subpath);
        if (eris_strcmp(dirent->name, subpath) == 0)
        {
            *size = dirent->size;

            return dirent;
        }
    }

    /* not found */
    return NULL;
}

int efs_ramfs_write(struct efs_file *fd, const void *buf, size_t count)
{
    struct ramfs_dirent *dirent;
    struct efs_ramfs *ramfs;

    dirent = (struct ramfs_dirent *)fd->vnode->data;
    Eris_ASSERT(dirent != NULL);

    ramfs = dirent->fs;
    Eris_ASSERT(ramfs != NULL);

    if (count + fd->pos > fd->vnode->size)
    {
        uint8_t *ptr;
        ptr = pvPortRealloc_efs(&(ramfs->memheap), dirent->data, fd->pos + count);
        if (ptr == NULL)
        {
            //eris_set_errno(-ENOMEM);                                                //1
            printf("Error: realloc failed!\n");
            return 0;
        }

        /* update dirent and file size */
        dirent->data = ptr;
        dirent->size = fd->pos + count;
        fd->vnode->size = dirent->size;
    }

    if (count > 0)
        eris_memcpy(dirent->data + fd->pos, buf, count);

    /* update file current position */
    fd->pos += count;

    return count;
}

int efs_ramfs_lseek(struct efs_file *file, off_t offset)
{
    if (offset <= (off_t)file->vnode->size)
    {
        file->pos = offset;

        return file->pos;
    }

    return -EIO;
}

int efs_ramfs_close(struct efs_file *file)
{
    Eris_ASSERT(file->vnode->ref_count > 0);
    if (file->vnode->ref_count > 1)
    {
        return 0;
    }

    file->vnode->data = NULL;

    return Eris_EOK;
}

int efs_ramfs_open(struct efs_file *file)
{
    size_t size;
    struct efs_ramfs *ramfs;
    struct ramfs_dirent *dirent;
    struct efs_filesystem *fs;

    Eris_ASSERT(file->vnode->ref_count > 0);
    if (file->vnode->ref_count > 1)
    {
        if (file->vnode->type == FT_DIRECTORY
                && !(file->flags & O_DIRECTORY))
        {
            return -ENOENT;
        }
        file->pos = 0;
        return 0;
    }

    fs = file->vnode->fs;

    ramfs = (struct efs_ramfs *)fs->data;
    Eris_ASSERT(ramfs != NULL);
    if (file->flags & O_DIRECTORY)
    {
        if (file->flags & O_CREAT)
        {
            return -ENOSPC;
        }

        /* open directory */
        dirent = efs_ramfs_lookup(ramfs, file->vnode->path, &size);
        if (dirent == NULL)
            return -ENOENT;
        if (dirent == &(ramfs->root)) /* it's root directory */
        {
            if (!(file->flags & O_DIRECTORY))
            {
                return -ENOENT;
            }
        }
        file->vnode->type = FT_DIRECTORY;
    }
    else
    {
        //printf("DEBUG: path: %s\n", file->vnode->path);
        dirent = efs_ramfs_lookup(ramfs, file->vnode->path, &size);
        if (dirent == &(ramfs->root)) /* it's root directory */
        {
            return -ENOENT;
        }

        if (dirent == NULL)
        {
            if (file->flags & O_CREAT || file->flags & O_WRONLY)
            {
                char *name_ptr;

                /* create a file entry */
                dirent = (struct ramfs_dirent *)
                        pvPortMalloc_efs(sizeof(struct ramfs_dirent),&(ramfs->memheap));
                if (dirent == NULL)
                {
                    return -ENOMEM;
                }

                /* remove '/' separator */
                name_ptr = file->vnode->path;
                while (*name_ptr == '/' && *name_ptr)
                {
                    name_ptr++;
                }
                strncpy(dirent->name, name_ptr, RAMFS_NAME_MAX);

                eris_list_init(&(dirent->list));
                dirent->data = NULL;
                dirent->size = 0;
                dirent->fs = ramfs;
                file->vnode->type = FT_REGULAR; //regular file

                /* add to the root directory */
                eris_list_insert_after(&(ramfs->root.list), &(dirent->list));
            }
            else
            {
                printf("[efs_ramfs.c] Error: file not found!\n");
                return -ENOENT;
            }
        }

        /* Creates a new file.
         * If the file is existing, it is truncated and overwritten.
         */
        if (file->flags & O_TRUNC)
        {
            dirent->size = 0;
            if (dirent->data != NULL)
            {
                vPortFree_efs(dirent->data, &(ramfs->memheap));
                dirent->data = NULL;
            }
        }
    }
    file->vnode->data = dirent;
    file->vnode->size = dirent->size;
    if (file->flags & O_APPEND)
    {
        file->pos = file->vnode->size;
    }
    else
    {
        file->pos = 0;
    }

    return 0;
}

int efs_ramfs_stat(struct efs_filesystem *fs,
                   const char            *path,
                   struct stat_efs           *st)
{
    size_t size;
    struct ramfs_dirent *dirent;
    struct efs_ramfs *ramfs;

    ramfs = (struct efs_ramfs *)fs->data;
    dirent = efs_ramfs_lookup(ramfs, path, &size);

    if (dirent == NULL)
        return -ENOENT;

    st->st_dev = 0;
    st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                  S_IWUSR | S_IWGRP | S_IWOTH;

    st->st_size = dirent->size;

    return Eris_EOK;
}

int efs_ramfs_getdents(struct efs_file *file,
                       struct dirent *dirp,
                       uint32_t    count)
{
    size_t index, end;
    struct dirent *d;
    struct ramfs_dirent *dirent;
    struct efs_ramfs *ramfs;

    dirent = (struct ramfs_dirent *)file->vnode->data;

    ramfs  = dirent->fs;
    Eris_ASSERT(ramfs != Eris_NULL);

    if (dirent != &(ramfs->root))
        return -EINVAL;

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -EINVAL;

    end = file->pos + count;
    index = 0;
    count = 0;
    for (dirent = eris_list_entry(dirent->list.next, struct ramfs_dirent, list);
         dirent != &(ramfs->root) && index < end;
         dirent = eris_list_entry(dirent->list.next, struct ramfs_dirent, list))
    {
        if (index >= (size_t)file->pos)
        {
            d = dirp + count;
            d->d_type = DT_REG;
            d->d_namlen = Eris_NAME_MAX;
            d->d_reclen = (eris_uint16_t)sizeof(struct dirent);
            strncpy(d->d_name, dirent->name, RAMFS_NAME_MAX);

            count += 1;
            file->pos += 1;
        }
        index += 1;
    }

    return count * sizeof(struct dirent);
}

int efs_ramfs_unlink(struct efs_filesystem *fs, const char *path)
{
    size_t size;
    struct efs_ramfs *ramfs;
    struct ramfs_dirent *dirent;

    ramfs = (struct efs_ramfs *)fs->data;
    Eris_ASSERT(ramfs != NULL);

    dirent = efs_ramfs_lookup(ramfs, path, &size);
    if (dirent == NULL)
        return -ENOENT;

    eris_list_remove(&(dirent->list));
    if (dirent->data != NULL)
        vPortFree_efs(dirent->data,&(ramfs->memheap));
    vPortFree_efs(dirent,&(ramfs->memheap));

    return Eris_EOK;
}

int efs_ramfs_rename(struct efs_filesystem *fs,
                     const char            *oldpath,
                     char            *newpath)
{
    struct ramfs_dirent *dirent;
    struct efs_ramfs *ramfs;
    size_t size;

    ramfs = (struct efs_ramfs *)fs->data;
    Eris_ASSERT(ramfs != NULL);

    dirent = efs_ramfs_lookup(ramfs, newpath, &size);
    if (dirent != NULL)
        return -EEXIST;

    dirent = efs_ramfs_lookup(ramfs, oldpath, &size);
    if (dirent == NULL)
        return -ENOENT;

    char * subpath = newpath;
    while (*subpath == '/' && *subpath)
        subpath ++;

    strncpy(dirent->name, subpath, RAMFS_NAME_MAX);

    return Eris_EOK;
}
static const struct efs_file_ops _ram_fops =
{
    efs_ramfs_open,
    efs_ramfs_close,
    efs_ramfs_ioctl,
    efs_ramfs_read,
    efs_ramfs_write,
    NULL,
    efs_ramfs_lseek,
    efs_ramfs_getdents,
};

static struct efs_filesystem_ops _ramfs =
{
    "ram",
    EFS_FS_FLAG_DEFAULT,
    &_ram_fops,

    efs_ramfs_mount,
    efs_ramfs_unmount,
    NULL, /* mkfs */
    efs_ramfs_statfs,

    efs_ramfs_unlink,
    efs_ramfs_stat,
    efs_ramfs_rename,
};

int efs_ramfs_init(void)
{
    /* register ram file system */
    efs_register(&_ramfs);

    return 0;
}
struct efs_ramfs *efs_ramfs_create(uint8_t *pool, size_t size)
{
    struct efs_ramfs *ramfs;
    uint8_t *data_ptr;
    eris_err_t result;

    size  = Eris_ALIGN_DOWN(size, Eris_ALIGN_SIZE);
    ramfs = (struct efs_ramfs *)pool;

    data_ptr = (uint8_t *)(ramfs + 1);
    size = size - sizeof(struct efs_ramfs);
    size = Eris_ALIGN_DOWN(size, Eris_ALIGN_SIZE);

    result = eris_memheap_init(&(ramfs->memheap), "ramfs", data_ptr, size);
    if (result != Eris_EOK)
        return NULL;
    /* detach this memheap object from the system */
    //eris_object_detach((eris_object_t) & (ramfs->memheap));

    /* initialize ramfs object */
    ramfs->magic = RAMFS_MAGIC;
    ramfs->memheap.parent.type = Eris_Object_Class_MemHeap | Eris_Object_Class_Static;

    /* initialize root directory */
    eris_memset(&(ramfs->root), 0x00, sizeof(ramfs->root));
    eris_list_init(&(ramfs->root.list));
    ramfs->root.size = 0;
    strcpy(ramfs->root.name, ".");
    ramfs->root.fs = ramfs;

    return ramfs;
}