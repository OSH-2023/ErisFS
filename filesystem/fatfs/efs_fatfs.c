#include "headers.h"

int fatfs_result_to_errno(FRESULT result) 
{
    int status = RT_EOK;

    switch (result)
    {
    case FR_OK:
        break;

    case FR_NO_FILE:
    case FR_NO_PATH:
    case FR_NO_FILESYSTEM:
        status = -ENOENT;
        break;

    case FR_INVALID_NAME:
        status = -EINVAL;
        break;

    case FR_EXIST:
    case FR_INVALID_OBJECT:
        status = -EEXIST;
        break;

    case FR_DISK_ERR:
    case FR_NOT_READY:
    case FR_INT_ERR:
        status = -EIO;
        break;

    case FR_WRITE_PROTECTED:
    case FR_DENIED:
        status = -EROFS;
        break;

    case FR_MKFS_ABORTED:
        status = -EINVAL;
        break;

    default:
        status = -1;
        break;
    }

    return status;
}

int efs_fatfs_open(struct efs_file *file)
{
    FRESULT result;
    char *drivers_fn; //drivers path? relative to device when FF_VOLUMES > 1
#if FF_VOLUMES > 1
    //TODO 
    drivers_fn = (char *)pvPortMalloc(256);
#else
    drivers_fn = file->vnode->path;
#endif   

    /* directory */
    if (file->flags & O_DIRECTORY)
    {
        DIR *dir = NULL;

        /* creat directory */
        if (file->flags & O_CREAT)
        {
            result = f_mkdir(drivers_fn);
            if (result != FR_OK)
            {
#if FF_VOLUMES > 1
                vPortFree(drivers_fn);
#endif
                printf("[efs_fatfs.c] efs_fatfs_open: failed to create directory!\n");
                return fatfs_result_to_dfs(result);
            }
        }

        /* open directory */
        dir = (DIR *)pvPortMalloc(sizeof(DIR));
        if (dir == NULL)
        {
#if FF_VOLUMES > 1
            vPortFree(drivers_fn);
#endif
            printf("[efs_fatfs.c] efs_fatfs_open: failed to alloc space for directory entry!\n");
            return -ENOMEM;
        }

        result = f_opendir(dir, drivers_fn);
#if FF_VOLUMES > 1  
        vPortFree(drivers_fn);
#endif
        if (result != FR_OK)
        {
            vPortfree(dir);
            printf("[efs_fatfs.c] efs_fatfs_open: failed to open the directory!\n");
            return fatfs_result_to_dfs(result);
        }

        //data point to dir entry
        file->data = dir; 
        return 0;
    }
    /* file */
    else 
    {
        FIL *fd = NULL;
        BYTE mode = FA_READ;

        /* Select mode according to flags */
        if (file->flags & O_WRONLY)
            mode |= FA_WRITE;
        if ((file->flags & O_ACCMODE) & O_RDWR)
            mode |= FA_WRITE;
        /* Opens the file, if it is existing. If not, a new file is created. */
        if (file->flags & O_CREAT)
            mode |= FA_OPEN_ALWAYS;
        /* Creates a new file. If the file is existing, it is truncated and OVERWRITTEN. */
        if (file->flags & O_TRUNC)
            mode |= FA_CREATE_ALWAYS;
        /* Creates a NEW file. The function fails if the file is already existing. */
        if (file->flags & O_EXCL)
            mode |= FA_CREATE_NEW;

        /* allocate a fd */
        fd = (FIL *)pvPortMalloc(sizeof(FIL));
        if (fd == NULL)
        {
#if FF_VOLUMES > 1
            vPortFree(drivers_fn);
#endif
            printf("[fatfs.c] efs_fatfs_open: failed to alloc space for file entry!\n");
            return -ENOMEM;
        }

        result = f_open(fd, drivers_fn, mode);
#if FF_VOLUMES > 1
        vPortFree(drivers_fn);
#endif
        if (result == FR_OK)
        {
            file->pos  = fd->fptr;
            file->vnode->size = f_size(fd);
            file->vnode->type = FT_REGULAR;
            file->data = fd;

            if (file->flags & O_APPEND)
            {
                f_lseek(fd, f_size(fd));
                file->pos = fd->fptr;
            }
        }
        else
        {
            vPortFree(fd);
            printf("[fatfs.c] efs_fatfs_open: failed to open the file!\n");
            return fatfs_result_to_dfs(result);
        }
    }
    return 0;
}

int efs_fatfs_close(struct efs_file *file)
{
    FRESULT result = FR_OK;
    Eris_ASSERT(file->vnode->ref_count > 0);
    
    if(file->vnode->ref_count > 1) 
    {
        return 0;
    }

    if(file->vnode->type == FT_DIRECTORY)
    { 
        // A directory's data is pointed to the DIR entry, free it.
        DIR *dir = NULL;
        dir = (DIR *)(file->data);
        if(dir != NULL)
        {
            printf("[fatfs.c] efs_fatfs_close: failed to find the dir!\n");
            return fatfs_result_to_errno(FR_NO_PATH);
        }
        else 
            vPortfree(dir);
    }
    else if(file->vnode->type == FT_REGULAR)
    {
        // A file's data is pointed to the FIL entry, free it.
        FIL *fd = NULL;
        fd = (FIL *)(file->data);
        
        if(fd == NULL)
        {
            printf("[fatfs.c] efs_fatfs_close: failed to find the file!\n");
            return fatfs_result_to_errno(FR_NO_FILE);
        }
        else 
        {
            result = f_close(fd);
            if (result == FR_OK)
                vPortFree(fd);
        }
    }

    return fatfs_result_to_dfs(result);
}

int efs_fatfs_ioctl(struct efs_file *file, int cmd, void *args)
{
    //TODO
    //relative with device.c
}


//return < 0 means error, return > 0 means the number of bytes read
int efs_fatfs_read(struct efs_file *file, void *buf, size_t len)
{
    FIL *fd = NULL;
    FRESULT result;
    UINT byte_read;

    if (file->vnode->type == FT_DIRECTORY)
    {
        printf("[fatfs.c] efs_fatfs_read: can't read from a directory.\n");
        return -EISDIR;
    }

    fd = (FIL *)(file->data);
    if(fd == NULL)
    {
        printf("[fatfs.c] efs_fatfs_read: failed to find file data!\n");
        return fatfs_result_to_errno(FR_NO_FILE);
    }
    else 
    {
        result = f_read(fd, buf, len, &byte_read);
        file->pos  = fd->fptr;

        if (result == FR_OK)
            return byte_read;
    }

    return fatfs_result_to_dfs(result);
}

//return < 0 means error, return > 0 means the number of bytes written
int efs_fatfs_write(struct efs_file *file, const void *buf, size_t len)
{
    FIL *fd = NULL;
    FRESULT result;
    UINT byte_write;

    if (file->vnode->type == FT_DIRECTORY)
    {
        printf("[fatfs.c] efs_fatfs_write: can't write into a directory.\n");
        return -EISDIR;
    }

    fd = (FIL *)(file->data);
    
    if(fd == NULL)
    {
        printf("[fatfs.c] efs_fatfs_write: failed to find file data!\n");
        return fatfs_result_to_errno(FR_NO_FILE);
    }
    else 
    {
        result = f_sync(fd);
    }

    return fatfs_result_to_dfs(result);
}

/* UNUSED
int efs_fatfs_flush(struct efs_file *file)
{
    FIL *fd = NULL;
    FRESULT result;

    fd = (FIL *)(file->data);
    if(fd == NULL)
    {
        printf("[fatfs.c] efs_fatfs_flush: failed to find file data!\n")
        return fatfs_result_to_errno(FR_NO_FILE);
    }
    else 
    {
        result = f_write(fd, buf, len, &byte_write);
        file->pos  = fd->fptr;
        file->vnode->size = f_size(fd);

        if (result == FR_OK)
            return byte_write;
    }

    result = f_sync(fd);
    return fatfs_result_to_dfs(result);
}

*/

//return < 0 means error, return > 0 means the current position
int efs_fatfs_lseek(struct efs_file *file, off_t offset)
{
    FRESULT result = FR_OK;
    if (file->vnode->type == FT_REGULAR)
    {
        FIL *fd = NULL;

        fd = (FIL *)(file->data);
        if(fd == NULL)
        {
            printf("[fatfs.c] efs_fatfs_lseek: failed to find file data!\n");
            return fatfs_result_to_errno(FR_NO_FILE);
        }
        else 
        {
            result = f_lseek(fd, offset);
            if (result == FR_OK)
            {
                file->pos = fd->fptr;
                return fd->fptr;
            }
        }
    }
    else if (file->vnode->type == FT_DIRECTORY)
    {
        DIR *dir = NULL;

        dir = (DIR *)(file->data);
        if(fd == NULL)
        {
            printf("[fatfs.c] efs_fatfs_lseek: failed to find dir data!\n");
            return fatfs_result_to_errno(FR_NO_PATH);
        }
        else 
        {
            result = f_seekdir(dir, offset / sizeof(struct dirent));
            if (result == FR_OK)
            {
                file->pos = offset;
                return file->pos;
            }
        }
    }

    return fatfs_result_to_dfs(result);
}

//return < 0 means error, return > 0 means the space in buf used.
int efs_fatfs_getdents(struct efs_file *file, struct dirent *dirp, uint32_t count)
{
    DIR *dir;
    FILINFO fno;
    FRESULT result;
    uint32_t index;
    struct dirent *d;

    dir = (DIR *)(file->data);
    if(dir == NULL)
    {
        printf("[fatfs.c] efs_fatfs_getdents: failed to find dir data!\n");
        return fatfs_result_to_errno(FR_NO_PATH);
    }

    /* make integer count */
    count = (count / sizeof(struct dirent)) * sizeof(struct dirent);
    if (count == 0)
    {
        printf("[efs_fatfs.c] efs_fatfs_getdents: read count is zero!\n");
        return -EINVAL;
    }

    index = 0;
    while (1)
    {
        char *fn;

        d = dirp + index;

        result = f_readdir(dir, &fno);
        if (result != FR_OK || fno.fname[0] == 0)
            break;

#if FF_USE_LFN
        fn = *fno.fname ? fno.fname : fno.altname;
#else
        fn = fno.fname;
#endif

        d->d_type = DT_UNKNOWN;
        if (fno.fattrib & AM_DIR)
            d->d_type = DT_DIR;
        else
            d->d_type = DT_REG;

        d->d_namlen = (uint8_t)strlen_efs(fn);
        d->d_reclen = (uint16_t)sizeof(struct dirent);
        strncpy(d->d_name, fn, DFS_PATH_MAX);

        index ++;
        if (index * sizeof(struct dirent) >= count)
            break;
    }

    if (index == 0)
    {
        printf("[efs_fatfs.c] efs_fatfs_getdents: failed while iterating through directory.\n");
    }

    file->pos += index * sizeof(struct dirent);
    return index * sizeof(struct dirent);
}