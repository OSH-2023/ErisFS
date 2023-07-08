#include "headers.h"

int efs_register(struct efs_filesystem_ops *ops)
{
    int ret = ERIS_EOK;
    struct efs_filesystem_ops **empty = NULL;
    struct efs_filesystem_ops **iter;

    /* lock filesystem */
    efs_lock();
    /* check if this filesystem was already registered */
    for (iter = &filesystem_operation_table[0];
            iter < &filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX]; iter ++)
    {
        /* find out an empty filesystem type entry */
        if (*iter == NULL)
            (empty == NULL) ? (empty = iter) : 0;
        else if (strcmp((*iter)->name, ops->name) == 0)
        {
            printf("The file system was already registered.\n");
            ret = -1;
            break;
        }
    }

    /* save the filesystem's operations */
    if (empty == NULL)
    {
        printf("There is no space to register this file system (%s).\n", ops->name);
        ret = -1;
    }
    else if (ret == ERIS_EOK)
    {
        *empty = ops;
    }

    efs_unlock();
    return ret;
}

struct efs_filesystem *efs_filesystem_lookup(const char *path)
{
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;
    uint32_t fspath, prefixlen;

    prefixlen = 0;

    if (!path) {
        printf("The path is NULL.\n");
        return NULL;
    }

    /* lock filesystem */
    efs_lock();

    /* lookup it in the filesystem table */
    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        if ((iter->path == NULL) || (iter->ops == NULL))
            continue;

        fspath = strlen_efs(iter->path);
        if ((fspath < prefixlen)
            || (strncmp(iter->path, path, fspath) != 0))
            continue;

        /* check next path separator */
        if (fspath > 1 && (strlen_efs(path) > (int)fspath) && (path[fspath] != '/'))
            continue;
        //printf("[efs_fs.c] path: %s\n", iter->path);

        fs = iter;
        prefixlen = fspath;
    }

    efs_unlock();
    //printf("[efs_fs.c] test\n");

    return fs;
}

const char *efs_filesystem_get_mounted_path(struct eris_device *device)
{
    const char *path = NULL;
    struct efs_filesystem *iter;

    efs_lock();
    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        /* find the mounted device */
        if (iter->ops == NULL) continue;
        else if (iter->dev_id == device)
        {
            path = iter->path;
            break;
        }
    }

    /* release filesystem_table lock */
    efs_unlock();

    return path;
}

int efs_filesystem_get_partition(struct efs_partition *part, uint8_t *buf, uint32_t pindex)
{
#define DPT_ADDRESS     0x1be       /* device partition offset in Boot Sector */
#define DPT_ITEM_SIZE   16          /* partition item size */

    uint8_t *dpt;
    uint8_t type;

    if(!part) {
        printf("The path is NULL.\n");
        return -ERIS_ERROR;
    }
    if(!buf) {
        printf("The buffer is NULL.\n");
        return -ERIS_ERROR;
    }

    dpt = buf + DPT_ADDRESS + pindex * DPT_ITEM_SIZE;

    /* check if it is a valid partition table */
    if ((*dpt != 0x80) && (*dpt != 0x00))
        return -ERIS_ERROR;

    /* get partition type */
    type = *(dpt + 4);
    if (type == 0)
        return -ERIS_ERROR;

    /* set partition information
     *    size is the number of 512-Byte */
    part->type = type;
    part->offset = *(dpt + 8) | *(dpt + 9) << 8 | *(dpt + 10) << 16 | *(dpt + 11) << 24;
    part->size = *(dpt + 12) | *(dpt + 13) << 8 | *(dpt + 14) << 16 | *(dpt + 15) << 24;

    printf("found part[%lud], begin: %ld, size: ",
               pindex, part->offset * 512);
    if ((part->size >> 11) == 0)
        printf("%d%s", part->size >> 1, "KB\n"); /* KB */
    else
    {
        unsigned int part_size;
        part_size = part->size >> 11;                /* MB */
        if ((part_size >> 10) == 0)
            printf("%d.%d%s", part_size, (part->size >> 1) & 0x3FF, "MB\n");
        else
            printf("%d.%d%s", part_size >> 10, part_size & 0x3FF, "GB\n");
    }

    return ERIS_EOK;
}

int efs_mount(const char *device_name, const char *path, const char *filesystemtype, unsigned long rwflag, const void *data)
{
    struct efs_filesystem_ops **ops;
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;
    char *fullpath = NULL;
    eris_device_t dev_id;

    /* open specific device */
    if (device_name == NULL)
    {
        /* which is a non-device filesystem mount */
        dev_id = NULL;
    } else if ((dev_id = eris_device_find(device_name)) == NULL) {
        printf("[efs_fs.c] no this device\n");
        return -1;
    }
    /* find out the specific filesystem */
    efs_lock();

    for (ops = &filesystem_operation_table[0];
            ops < &filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX]; ops++)
    {
        if ((*ops != NULL) && (strncmp((*ops)->name, filesystemtype, strlen_efs((*ops)->name)) == 0))
            break;
        printf("filesystem: %s\n", (*ops)->name);
    }

    efs_unlock();

    if (ops == &filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX])
    {
        /* can't find filesystem */
        printf("Can't find filesystem.\n");
        return -1;
    }

    /* check if there is mount implementation */
    if ((*ops == NULL) || ((*ops)->mount == NULL))
    {
        printf("mount failed.\n");
        return -1;
    }

    /* make full path for special file */
    fullpath = efs_normalize_path(NULL, path);
    if (fullpath == NULL) /* not an abstract path */
    {
        printf("Fullpath Wrong.\n");
        return -1;
    }

    /* Check if the path exists or not, raw APIs call, fixme */
    if ((strcmp(fullpath, "/") != 0) && (strcmp(fullpath, "/dev") != 0))
    {
        struct efs_file fd;

        fd_init(&fd);
        if (efs_file_open(&fd, fullpath, O_RDONLY | O_DIRECTORY) < 0)
        {
            vPortFree(fullpath);
            printf("The path does not exist \n");
            return -1;
        }
        efs_file_close(&fd);
    }

    /* check whether the file system mounted or not  in the filesystem table
     * if it is unmounted yet, find out an empty entry */
    efs_lock();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        /* check if it is an empty filesystem table entry? if it is, save fs */
        if (iter->ops == NULL)
            (fs == NULL) ? (fs = iter) : 0;
        /* check if the PATH is mounted */
        else if (strcmp(iter->path, path) == 0)
        {
            printf("The path has been mounted.\n");
            goto err1;
        }
    }

    if ((fs == NULL) && (iter == &filesystem_table[EFS_FILESYSTEMS_MAX]))
    {
        printf("There is no space to mount this file system (%s).\n", filesystemtype);
        goto err1;
    }

    /* register file system */
    fs->path   = fullpath;
    fs->ops    = *ops;
    fs->dev_id = dev_id;
    /* For UFS, record the real filesystem name */
    fs->data = (void *) filesystemtype;

    /* release filesystem_table lock */
    efs_unlock();

    /* open device, but do not check the status of device */
    if (dev_id != NULL)
    {
        if (eris_device_open(fs->dev_id,
                            ERIS_DEVICE_OFLAG_RDWR) != ERIS_EOK)
        {
            /* The underlying device has error, clear the entry. */
            efs_lock();
            memset(fs, 0, sizeof(struct efs_filesystem));

            goto err1;
        }
    }

    /* call mount of this filesystem */
    if ((*ops)->mount(fs, rwflag, data) < 0)
    {
        /* close device */
        if (dev_id != NULL)
            eris_device_close(fs->dev_id);

        /* mount failed */
        efs_lock();
        /* clear filesystem table entry */
        memset(fs, 0, sizeof(struct efs_filesystem));

        goto err1;
    }

    return 0;

err1:
    efs_unlock();
    vPortFree(fullpath);

    return -1;
}

int efs_unmount(const char *specialfile)
{
    char *fullpath;
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;

    fullpath = efs_normalize_path(NULL, specialfile);
    if (fullpath == NULL)
    {
        printf("Wrong path\n");
        return -1;
    }

    /* lock filesystem */
    efs_lock();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        /* check if the PATH is mounted */
        if ((iter->path != NULL) && (strcmp(iter->path, fullpath) == 0))
        {
            fs = iter;
            break;
        }
    }

    if (fs == NULL ||
        fs->ops->unmount == NULL ||
        fs->ops->unmount(fs) < 0)
    {
        goto err1;
    }

    /* close device, but do not check the status of device */
    if (fs->dev_id != NULL)
        eris_device_close(fs->dev_id);

    if (fs->path != NULL)
        vPortFree(fs->path);

    /* clear this filesystem table entry */
    memset(fs, 0, sizeof(struct efs_filesystem));

    efs_unlock();
    vPortFree(fullpath);

    return 0;

err1:
    efs_unlock();
    vPortFree(fullpath);

    return -1;
}

int efs_mkfs(const char *fs_name, const char *device_name)
{
    int index;
    eris_device_t dev_id = NULL;

    /* check device name, and it should not be NULL */
    if (device_name != NULL)
        dev_id = eris_device_find(device_name);

    if (dev_id == NULL)
    {
        printf("Device (%s) was not found\r\n", device_name);
        return -1;
    }

    /* lock file system */
    efs_lock();
    /* find the file system operations */
    for (index = 0; index < EFS_FILESYSTEM_TYPES_MAX; index ++)
    {
        if (filesystem_operation_table[index] != NULL &&
            strncmp(filesystem_operation_table[index]->name, fs_name,
                strlen_efs(filesystem_operation_table[index]->name)) == 0)
            break;
    }
    efs_unlock();

    if (index < EFS_FILESYSTEM_TYPES_MAX)
    {
        /* find file system operation */
        const struct efs_filesystem_ops *ops = filesystem_operation_table[index];
        if (ops->mkfs == NULL)
        {
            printf("The file system (%s) mkfs function was not implement\n", fs_name);
            return -1;
        }

        return ops->mkfs(dev_id, fs_name);
    }

    printf("File system (%s) was not found.\n", fs_name);

    return -1;
}

int efs_statfs(const char *path, struct statfs *buffer)
{
    struct efs_filesystem *fs;

    fs = efs_filesystem_lookup(path);
    if (fs != NULL)
    {
        if (fs->ops->statfs != NULL)
            return fs->ops->statfs(fs, buffer);
        else 
        {
            printf("[efs_fs.c] efs_statfs: statfs is NULL\n");
            return -1;
        }
    }
    printf("[efs_fs.c] efs_statfs: no corresponding filesystem\n");
    return -1;
}
