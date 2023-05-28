#include <efs_fs.h>
#include <efs_file.h>


int efs_register(const struct efs_filesystem_ops *ops)
{
    int ret = ERIS_EOK;
    const struct efs_filesystem_ops **empty = NULL;
    const struct efs_filesystem_ops **iter;

    /* lock filesystem */
    dfs_lock();
    /* check if this filesystem was already registered */
    for (iter = &filesystem_operation_table[0];
            iter < &filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX]; iter ++)
    {
        /* find out an empty filesystem type entry */
        if (*iter == NULL)
            (empty == NULL) ? (empty = iter) : 0;
        else if (strcmp((*iter)->name, ops->name) == 0)
        {
            rt_set_errno(-EEXIST); // TODO
            ret = -1;
            break;
        }
    }

    /* save the filesystem's operations */
    if (empty == NULL)
    {
        rt_set_errno(-ENOSPC);  // TODO
        LOG_E("There is no space to register this file system (%s).", ops->name);   // TODO
        ret = -1;
    }
    else if (ret == ERIS_EOK)
    {
        *empty = ops;
    }

    dfs_unlock();
    return ret;
}

struct efs_filesystem *efs_filesystem_lookup(const char *path)
{
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;
    uint32_t fspath, prefixlen;

    prefixlen = 0;

    RT_ASSERT(path);

    /* lock filesystem */
    dfs_lock();

    /* lookup it in the filesystem table */
    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        if ((iter->path == NULL) || (iter->ops == NULL))
            continue;

        fspath = strlen(iter->path);
        if ((fspath < prefixlen)
            || (strncmp(iter->path, path, fspath) != 0))
            continue;

        /* check next path separator */
        if (fspath > 1 && (strlen(path) > fspath) && (path[fspath] != '/'))
            continue;

        fs = iter;
        prefixlen = fspath;
    }

    dfs_unlock();

    return fs;
}

const char *efs_filesystem_get_mounted_path(struct eris_device *device)
{
    const char *path = NULL;
    struct efs_filesystem *iter;

    dfs_lock();
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
    dfs_unlock();

    return path;
}

int efs_filesystem_get_partition(struct efs_partition *part, uint8_t *buf, uint32_t pindex)
{
#define DPT_ADDRESS     0x1be       /* device partition offset in Boot Sector */
#define DPT_ITEM_SIZE   16          /* partition item size */

    uint8_t *dpt;
    uint8_t type;

    RT_ASSERT(part != NULL);
    RT_ASSERT(buf != NULL);

    dpt = buf + DPT_ADDRESS + pindex * DPT_ITEM_SIZE;

    /* check if it is a valid partition table */
    if ((*dpt != 0x80) && (*dpt != 0x00))
        return -EIO;

    /* get partition type */
    type = *(dpt + 4);
    if (type == 0)
        return -EIO;

    /* set partition information
     *    size is the number of 512-Byte */
    part->type = type;
    part->offset = *(dpt + 8) | *(dpt + 9) << 8 | *(dpt + 10) << 16 | *(dpt + 11) << 24;
    part->size = *(dpt + 12) | *(dpt + 13) << 8 | *(dpt + 14) << 16 | *(dpt + 15) << 24;

    rt_kprintf("found part[%d], begin: %d, size: ",
               pindex, part->offset * 512);
    if ((part->size >> 11) == 0)
        rt_kprintf("%d%s", part->size >> 1, "KB\n"); /* KB */
    else
    {
        unsigned int part_size;
        part_size = part->size >> 11;                /* MB */
        if ((part_size >> 10) == 0)
            rt_kprintf("%d.%d%s", part_size, (part->size >> 1) & 0x3FF, "MB\n");
        else
            rt_kprintf("%d.%d%s", part_size >> 10, part_size & 0x3FF, "GB\n");
    }

    return ERIS_EOK;
}

int efs_mount(const char *device_name, const char *path, const char *filesystemtype, unsigned long rwflag, const void *data)
{
    const struct efs_filesystem_ops **ops;
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;
    char *fullpath = NULL;
    eris_device_t dev_id;

    /* open specific device */
    if (device_name == NULL)
    {
        /* which is a non-device filesystem mount */
        dev_id = NULL;
    }
    else if ((dev_id = rt_device_find(device_name)) == NULL)
    {
        /* no this device */
        rt_set_errno(-ENODEV);  // TODO
        return -1;
    }

    /* find out the specific filesystem */
    dfs_lock();

    for (ops = &filesystem_operation_table[0];
            ops < &filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX]; ops++)
        if ((*ops != NULL) && (strncmp((*ops)->name, filesystemtype, strlen((*ops)->name)) == 0))
            break;

    dfs_unlock();

    if (ops == &filesystem_operation_table[EFS_FILESYSTEM_TYPES_MAX])
    {
        /* can't find filesystem */
        rt_set_errno(-ENODEV);  // TODO
        return -1;
    }

    /* check if there is mount implementation */
    if ((*ops == NULL) || ((*ops)->mount == NULL))
    {
        rt_set_errno(-ENOSYS);  // TODO
        return -1;
    }

    /* make full path for special file */
    fullpath = dfs_normalize_path(NULL, path);
    if (fullpath == NULL) /* not an abstract path */
    {
        rt_set_errno(-ENOTDIR); // TODO
        return -1;
    }

    /* Check if the path exists or not, raw APIs call, fixme */
    if ((strcmp(fullpath, "/") != 0) && (strcmp(fullpath, "/dev") != 0))
    {
        struct efs_file fd;

        fd_init(&fd);
        if (dfs_file_open(&fd, fullpath, O_RDONLY | O_DIRECTORY) < 0)   // TODO
        {
            rt_free(fullpath);
            rt_set_errno(-ENOTDIR); // TODO

            return -1;
        }
        dfs_file_close(&fd);
    }

    /* check whether the file system mounted or not  in the filesystem table
     * if it is unmounted yet, find out an empty entry */
    dfs_lock();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        /* check if it is an empty filesystem table entry? if it is, save fs */
        if (iter->ops == NULL)
            (fs == NULL) ? (fs = iter) : 0;
        /* check if the PATH is mounted */
        else if (strcmp(iter->path, path) == 0)
        {
            rt_set_errno(-EINVAL);  // TODO
            goto err1;
        }
    }

    if ((fs == NULL) && (iter == &filesystem_table[EFS_FILESYSTEMS_MAX]))
    {
        rt_set_errno(-ENOSPC);  // TODO
        LOG_E("There is no space to mount this file system (%s).", filesystemtype);
        goto err1;
    }

    /* register file system */
    fs->path   = fullpath;
    fs->ops    = *ops;
    fs->dev_id = dev_id;
    /* For UFS, record the real filesystem name */
    fs->data = (void *) filesystemtype;

    /* release filesystem_table lock */
    dfs_unlock();

    /* open device, but do not check the status of device */
    if (dev_id != NULL)
    {
        if (eris_device_open(fs->dev_id,    // TODO
                           RT_DEVICE_OFLAG_RDWR) != ERIS_EOK)
        {
            /* The underlying device has error, clear the entry. */
            dfs_lock();
            rt_memset(fs, 0, sizeof(struct efs_filesystem));    // TODO

            goto err1;
        }
    }

    /* call mount of this filesystem */
    if ((*ops)->mount(fs, rwflag, data) < 0)
    {
        /* close device */
        if (dev_id != NULL)
            rt_device_close(fs->dev_id);    // TODO

        /* mount failed */
        dfs_lock();
        /* clear filesystem table entry */
        rt_memset(fs, 0, sizeof(struct efs_filesystem));

        goto err1;
    }

    return 0;

err1:
    dfs_unlock();
    rt_free(fullpath);  // TODO

    return -1;
}

int efs_unmount(const char *specialfile)
{
    char *fullpath;
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;

    fullpath = dfs_normalize_path(NULL, specialfile);
    if (fullpath == NULL)
    {
        rt_set_errno(-ENOTDIR); // TODO

        return -1;
    }

    /* lock filesystem */
    dfs_lock();

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
        rt_device_close(fs->dev_id);    // TODO

    if (fs->path != NULL)
        rt_free(fs->path);  // TODO

    /* clear this filesystem table entry */
    rt_memset(fs, 0, sizeof(struct efs_filesystem)); // TODO

    dfs_unlock();
    rt_free(fullpath);  // TODO

    return 0;

err1:
    dfs_unlock();
    rt_free(fullpath);  // TODO

    return -1;
}


int efs_mkfs(const char *fs_name, const char *device_name)
{
    int index;
    eris_device_t dev_id = NULL;

    /* check device name, and it should not be NULL */
    if (device_name != NULL)
        dev_id = rt_device_find(device_name);

    if (dev_id == NULL)
    {
        rt_set_errno(-ENODEV);  // TODO
        LOG_E("Device (%s) was not found", device_name);    // TODO
        return -1;
    }

    /* lock file system */
    dfs_lock();
    /* find the file system operations */
    for (index = 0; index < EFS_FILESYSTEM_TYPES_MAX; index ++)
    {
        if (filesystem_operation_table[index] != NULL &&
            strncmp(filesystem_operation_table[index]->name, fs_name,
                strlen(filesystem_operation_table[index]->name)) == 0)
            break;
    }
    dfs_unlock();

    if (index < EFS_FILESYSTEM_TYPES_MAX)
    {
        /* find file system operation */
        const struct efs_filesystem_ops *ops = filesystem_operation_table[index];
        if (ops->mkfs == NULL)
        {
            LOG_E("The file system (%s) mkfs function was not implement", fs_name); // TODO
            rt_set_errno(-ENOSYS);  // TODO
            return -1;
        }

        return ops->mkfs(dev_id, fs_name);
    }

    LOG_E("File system (%s) was not found.", fs_name);  // TODO

    return -1;
}

int efs_statfs(const char *path, struct statfs *buffer)
{
    struct efs_filesystem *fs;

    fs = dfs_filesystem_lookup(path);
    if (fs != NULL)
    {
        if (fs->ops->statfs != NULL)
            return fs->ops->statfs(fs, buffer);
    }

    rt_set_errno(-ENOSYS);  // TODO
    return -1;
}

//#ifdef RT_USING_DFS_MNTTABLE
int efs_mount_table(void)
{
    int index = 0;

    while (1)
    {
        if (mount_table[index].path == NULL) break;

        if (efs_mount(mount_table[index].device_name,
                      mount_table[index].path,
                      mount_table[index].filesystemtype,
                      mount_table[index].rwflag,
                      mount_table[index].data) != 0)
        {
            LOG_E("mount fs[%s] on %s failed.\n", mount_table[index].filesystemtype,
                       mount_table[index].path);    // TODO
            return -ERIS_ERROR;
        }

        index ++;
    }
    return 0;
}
INIT_ENV_EXPORT(dfs_mount_table);   // TODO

int efs_mount_device(eris_device_t dev)
{
  int index = 0;

  if(dev == NULL) {
    rt_kprintf("the device is NULL to be mounted.\n");
    return -ERIS_ERROR;
  }

  while (1)
  {
    if (mount_table[index].path == NULL) break;

    if(strcmp(mount_table[index].device_name, dev->parent.name) == 0) {
      if (efs_mount(mount_table[index].device_name,
                    mount_table[index].path,
                    mount_table[index].filesystemtype,
                    mount_table[index].rwflag,
                    mount_table[index].data) != 0)
      {
        LOG_E("mount fs[%s] device[%s] to %s failed.\n", mount_table[index].filesystemtype, dev->parent.name,
                   mount_table[index].path);    // TODO
        return -ERIS_ERROR;
      } else {
        LOG_D("mount fs[%s] device[%s] to %s ok.\n", mount_table[index].filesystemtype, dev->parent.name,
                   mount_table[index].path);    // TODO
        return ERIS_EOK;
      }
    }

    index ++;
  }

  rt_kprintf("can't find device:%s to be mounted.\n", dev->parent.name);    // TODO
  return -ERIS_ERROR;
}

int efs_unmount_device(eris_device_t dev)
{
    struct efs_filesystem *iter;
    struct efs_filesystem *fs = NULL;

    /* lock filesystem */
    dfs_lock();

    for (iter = &filesystem_table[0];
            iter < &filesystem_table[EFS_FILESYSTEMS_MAX]; iter++)
    {
        /* check if the PATH is mounted */
        if (strcmp(iter->dev_id->parent.name, dev->parent.name) == 0)
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
        rt_device_close(fs->dev_id);

    if (fs->path != NULL)
        rt_free(fs->path);

    /* clear this filesystem table entry */
    rt_memset(fs, 0, sizeof(struct efs_filesystem));

    dfs_unlock();

    return 0;

err1:
    dfs_unlock();

    return -1;
}

//#endif

/**@}*/
