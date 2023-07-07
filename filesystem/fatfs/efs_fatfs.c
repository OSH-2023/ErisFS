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

//fatfs funtions about fs
static int get_disk(eris_device_t id)
{
    int index;

    for (index = 0; index < FF_VOLUMES; index ++)
    {
        if (disk[index] == id)
            return index;
    }

    return -1;
}

int efs_fatfs_mount(struct efs_filesystem *fs, unsigned long rwflag, const void *data)
{
    FATFS *fat;
    FRESULT result;
    int index;
    struct rt_device_blk_geometry geometry;
    char logic_nbr[3] = {'0',':', 0};

    /* get an empty position */
    index = get_disk(NULL);
    if (index == -1)
    {
        printf('[efs_fatfs.c] failed to get disk in efs_fatfs_mount!\n');
        return -1;
    }
        
    logic_nbr[0] = '0' + index;

    /* save device */
    disk[index] = fs->dev_id;

    fat = (FATFS *)pvPortMalloc(sizeof(FATFS));
    if (fat == NULL)
    {
        disk[index] = NULL;
        printf('[efs_fatfs.c] failed to allocate space for fat in efs_fatfs_mount!\n');
        return -1;
    }

    /* mount fatfs, always 0 logic driver */
    result = f_mount(fat, (const TCHAR *)logic_nbr, 1);
    if (result == FR_OK)
    {
        char drive[8];
        DIR *dir;

        //rt_snprintf(drive, sizeof(drive), "%d:/", index);////////////////?
        dir = (DIR *)pvPortMalloc(sizeof(DIR));
        if (dir == NULL)
        {
            f_mount(NULL, (const TCHAR *)logic_nbr, 1);
            disk[index] = NULL;
            vPortFree(fat);
            printf('[efs_fatfs.c] failed to allocate space for dir in efs_fatfs_mount!\n');
            return -1;
        }

        /* open the root directory to test whether the fatfs is valid */
        result = f_opendir(dir, drive);
        if (result != FR_OK)
            goto __err;

        /* mount succeed! */
        fs->data = fat;
        vPortFree(dir);
        return 0;
    }

__err:
    f_mount(NULL, (const TCHAR *)logic_nbr, 1);
    disk[index] = NULL;
    vPortFree(fat);
    return fatfs_result_to_errno(result);
}

int efs_fatfs_unmount(struct efs_filesystem *fs)
{
    FATFS *fat;
    FRESULT result;
    int  index;
    char logic_nbr[3] = {'0',':', 0};

    fat = (FATFS *)fs->data;

    if (fat == NULL)
    {
        printf('[efs_fatfs.c] failed to fetch fat in efs_fatfs_unmount!\n');
        return -1;
    }

    /* find the device index and then umount it */
    index = get_disk(fs->dev_id);
    if (index == -1) /* not found */
    {
        printf('[efs_fatfs.c] failed to get disk in efs_fatfs_unmount!\n');
        return -1;
    }

    logic_nbr[0] = '0' + index;
    result = f_mount(NULL, logic_nbr, (unsigned char)0);
    if (result != FR_OK)
        return fatfs_result_to_errno(result);

    fs->data = NULL;
    disk[index] = NULL;
    vPortFree(fat);

    return 0;
}

int efs_fatfs_mkfs(eris_device_t dev_id, const char *fs_name)
{
#define FSM_STATUS_INIT            0
#define FSM_STATUS_USE_TEMP_DRIVER 1
    FATFS *fat = NULL;
    unsigned char *work;
    int flag;
    FRESULT result;
    int index;
    char logic_nbr[3] = {'0',':', 0};
    MKFS_PARM opt;

    work = pvPortMalloc(FF_MAX_SS);
    if(NULL == work) {
        printf('[efs_fatfs.c] failed to allocate space for work in efs_fatfs_mkfs!\n');
        return -1;
    }

    if (dev_id == NULL)
    {
        vPortFree(work); /* release memory */
        return -2;
    }

    /* if the device is already mounted, then just do mkfs to the drv,
     * while if it is not mounted yet, then find an empty drive to do mkfs
     */

    flag = FSM_STATUS_INIT;
    index = get_disk(dev_id);
    if (index == -1)
    {
        /* not found the device id */
        index = get_disk(NULL);
        if (index == -1)
        {
            /* no space to store an temp driver */
            printf("[efs_fatfs.c]sorry, there is no space to do mkfs! \n");
            vPortFree(work); /* release memory */
            return -9;
        }
        else
        {
            fat = (FATFS *)pvPortMalloc(sizeof(FATFS));
            if (fat == NULL)
            {
                vPortFree(work); /* release memory */
                printf('[efs_fatfs.c] failed to allocate space for fat in efs_fatfs_mkfs!\n');
                return -1;
            }

            flag = FSM_STATUS_USE_TEMP_DRIVER;

            disk[index] = dev_id;
            /* try to open device */
            eris_device_open(dev_id, 3);

            /* just fill the FatFs[vol] in ff.c, or mkfs will failded!
             * consider this condition: you just umount the fatfs fat,
             * then the space in FatFs[index] is released, and now do mkfs
             * on the disk, you will get a failure. so we need f_mount here,
             * just fill the FatFS[index] in fatfs fatfs to make mkfs work.
             */
            logic_nbr[0] = '0' + index;
            f_mount(fat, logic_nbr, (unsigned char)index);
        }
    }
    else
    {
        logic_nbr[0] = '0' + index;
    }

    /* [IN] Logical drive number */
    /* [IN] Format options */
    /* [-]  Working buffer */
    /* [IN] Size of working buffer */
    memset(&opt, 0, sizeof(opt));
    opt.fmt = FM_ANY|FM_SFD;
    result = f_mkfs(logic_nbr, &opt, work, FF_MAX_SS);
    vPortFree(work); 
    work = NULL;

    /* check flag status, we need clear the temp driver stored in disk[] */
    if (flag == FSM_STATUS_USE_TEMP_DRIVER)
    {
        vPortFree(fat);
        f_mount(NULL, logic_nbr, (unsigned char)index);
        disk[index] = NULL;
        /* close device */
        eris_device_close(dev_id);
    }

    if (result != FR_OK)
    {
        printf("[efs_fatfs.c]format error, result=%d\n", result);
        return fatfs_result_to_errno(result);
    }

    return 0;
}

int efs_fatfs_statfs(struct efs_filesystem *fs, struct statfs *buf)
{
    FATFS *f;
    FRESULT res;
    char driver[4];
    unsigned long fre_clust, fre_sect, tot_sect;

    if (fs == NULL)
    {
        printf('[efs_fatfs.c] failed to get valid fs in efs_fatfs_statfs!\n');
        return -1;
    }
    if (buf == NULL)
    {
        printf('[efs_fatfs.c] failed to get valid buf in efs_fatfs_statfs!\n');
        return -1;
    }

    f = (FATFS *)fs->data;

    printf(driver, sizeof(driver), "%d:", f->pdrv);
    res = f_getfree(driver, &fre_clust, &f);
    if (res)
        return fatfs_result_to_errno(res);

    /* Get total sectors and free sectors */
    tot_sect = (f->n_fatent - 2) * f->csize;
    fre_sect = fre_clust * f->csize;

    buf->f_bfree = fre_sect;
    buf->f_blocks = tot_sect;
#if FF_MAX_SS != 512
    buf->f_bsize = f->ssize;
#else
    buf->f_bsize = 512;
#endif

    return 0;
}

int efs_fatfs_unlink(struct efs_filesystem *fs, const char *path)
{
    FRESULT result;

#if FF_VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int fatfs_get_vol(FATFS * fat);

    /* add path for fatfs FatFS driver support */
    drivers_fn = (char *)pvPortMalloc(256);
    if (drivers_fn == NULL)
    {
        printf('[efs_fatfs.c] failed to allocate space for drivers_fn in efs_fatfs_unlink!\n');
        return -1;
    }
    printf("unlink:%d,%s", vol, path);
#else
    const char *drivers_fn;
    drivers_fn = path;
#endif

    result = f_unlink(drivers_fn);
#if FF_VOLUMES > 1
    vPortFree(drivers_fn);
#endif
    return fatfs_result_to_errno(result);
}

int efs_fatfs_rename(struct efs_filesystem *fs, const char *oldpath, const char *newpath)
{
    FRESULT result;

#if FF_VOLUMES > 1
    char *drivers_oldfn;
    const char *drivers_newfn;
    int vol;
    //extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */

    drivers_oldfn = (char *)rt_malloc(256);
    if (drivers_fn == NULL)
    {
        printf('[efs_fatfs.c] failed to allocate space for drivers_fn in efs_fatfs_rename!\n');
        return -1;
    }
    drivers_newfn = newpath;

    printf("rename:%d,%s", vol, oldpath);
#else
    const char *drivers_oldfn, *drivers_newfn;

    drivers_oldfn = oldpath;
    drivers_newfn = newpath;
#endif

    result = f_rename(drivers_oldfn, drivers_newfn);
#if FF_VOLUMES > 1
    vPortFree(drivers_oldfn);
#endif
    return fatfs_result_to_errno(result);
}

int efs_fatfs_stat(struct efs_filesystem *fs, const char *path, struct stat *st)
{
    FATFS  *f;
    FILINFO file_info;
    FRESULT result;

    f = (FATFS *)fs->data;

#if FF_VOLUMES > 1
    int vol;
    char *drivers_fn;
    //extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    drivers_fn = (char *)rt_malloc(256);
    if (drivers_fn == NULL)
    {
        printf('[efs_fatfs.c] failed to allocate space for drivers_fn in efs_fatfs_stat!\n');
        return -1;
    }

    printf("stat:%d,%s", vol, path);
#else
    const char *drivers_fn;
    drivers_fn = path;
#endif

    result = f_stat(drivers_fn, &file_info);
#if FF_VOLUMES > 1
    vPortFree(drivers_fn);
#endif
    if (result == FR_OK)
    {
        /* convert to efs stat structure */
        st->st_dev = 0;

        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                      S_IWUSR | S_IWGRP | S_IWOTH;
        if (file_info.fattrib & AM_DIR)
        {
            st->st_mode &= ~S_IFREG;
            st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        }
        if (file_info.fattrib & AM_RDO)
            st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);

        st->st_size  = file_info.fsize;
        st->st_blksize = f->csize * SS(f);
        if (file_info.fattrib & AM_ARC)
        {
            st->st_blocks = file_info.fsize ? ((file_info.fsize - 1) / SS(f) / f->csize + 1) : 0;
            st->st_blocks *= (st->st_blksize / 512);  // man say st_blocks is number of 512B blocks allocated
        }
        else
        {
            st->st_blocks = f->csize;
        }
        /* get st_mtime. */
        {
            struct tm tm_file;
            int year, mon, day, hour, min, sec;
            unsigned short tmp;

            tmp = file_info.fdate;
            day = tmp & 0x1F;           /* bit[4:0] Day(1..31) */
            tmp >>= 5;
            mon = tmp & 0x0F;           /* bit[8:5] Month(1..12) */
            tmp >>= 4;
            year = (tmp & 0x7F) + 1980; /* bit[15:9] Year origin from 1980(0..127) */

            tmp = file_info.ftime;
            sec = (tmp & 0x1F) * 2;     /* bit[4:0] Second/2(0..29) */
            tmp >>= 5;
            min = tmp & 0x3F;           /* bit[10:5] Minute(0..59) */
            tmp >>= 6;
            hour = tmp & 0x1F;          /* bit[15:11] Hour(0..23) */

            memset(&tm_file, 0, sizeof(tm_file));
            tm_file.tm_year = year - 1900; /* Years since 1900 */
            tm_file.tm_mon  = mon - 1;     /* Months *since* january: 0-11 */
            tm_file.tm_mday = day;         /* Day of the month: 1-31 */
            tm_file.tm_hour = hour;        /* Hours since midnight: 0-23 */
            tm_file.tm_min  = min;         /* Minutes: 0-59 */
            tm_file.tm_sec  = sec;         /* Seconds: 0-59 */

            st->st_mtime = timegm(&tm_file);
        } /* get st_mtime. */
    }

    return fatfs_result_to_errno(result);
}

time_t timegm(struct tm * const t)
{
    time_t day;
    time_t i;
    time_t years;

    if(t == NULL)
    {
        printf('[efs_fatfs.c] failed to get t in timegm!\n');
        return (time_t)-1;
    }

    years = (time_t)t->tm_year - 70;
    if (t->tm_sec > 60)         /* seconds after the minute - [0, 60] including leap second */
    {
        t->tm_min += t->tm_sec / 60;
        t->tm_sec %= 60;
    }
    if (t->tm_min >= 60)        /* minutes after the hour - [0, 59] */
    {
        t->tm_hour += t->tm_min / 60;
        t->tm_min %= 60;
    }
    if (t->tm_hour >= 24)       /* hours since midnight - [0, 23] */
    {
        t->tm_mday += t->tm_hour / 24;
        t->tm_hour %= 24;
    }
    if (t->tm_mon >= 12)        /* months since January - [0, 11] */
    {
        t->tm_year += t->tm_mon / 12;
        t->tm_mon %= 12;
    }
    while (t->tm_mday > __spm[1 + t->tm_mon])
    {
        if (t->tm_mon == 1 && __isleap(t->tm_year + 1900))
        {
            --t->tm_mday;
        }
        t->tm_mday -= __spm[t->tm_mon];
        ++t->tm_mon;
        if (t->tm_mon > 11)
        {
            t->tm_mon = 0;
            ++t->tm_year;
        }
    }

    if (t->tm_year < 70)
    {
        printf('[efs_fatfs.c] failed to get valid tm_year in timegm!\n');
        return (time_t) -1;
    }

    /* Days since 1970 is 365 * number of years + number of leap years since 1970 */
    day = years * 365 + (years + 1) / 4;

    /* After 2100 we have to substract 3 leap years for every 400 years
     This is not intuitive. Most mktime implementations do not support
     dates after 2059, anyway, so we might leave this out for it's
     bloat. */
    if (years >= 131)
    {
        years -= 131;
        years /= 100;
        day -= (years >> 2) * 3 + 1;
        if ((years &= 3) == 3)
            years--;
        day -= years;
    }

    day += t->tm_yday = __spm[t->tm_mon] + t->tm_mday - 1 +
                        (__isleap(t->tm_year + 1900) & (t->tm_mon > 1));

    /* day is now the number of days since 'Jan 1 1970' */
    i = 7;
    t->tm_wday = (int)((day + 4) % i); /* Sunday=0, Monday=1, ..., Saturday=6 */

    i = 24;
    day *= i;
    i = 60;
    return ((day + t->tm_hour) * i + t->tm_min) * i + t->tm_sec;
}

static int __isleap(int year)
{
    /* every fourth year is a leap year except for century years that are
     * not divisible by 400. */
    /*  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); */
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

int efs_fatfs_init(void)
{
    /* register fatfs file system */
    efs_register(&efs_fatfs);

    return 0;
}