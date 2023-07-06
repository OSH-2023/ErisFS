#include <headers.h>
/* results:
 *  -1, no space to install fatfs driver
 *  >= 0, there is an space to install fatfs driver
 */
static int elm_result_to_efs(FRESULT result)
{
    int status = 0;

    switch (result)
    {
    case FR_OK:
        break;

    case FR_NO_FILE:
    case FR_NO_PATH:
    case FR_NO_FILESYSTEM:
        status = -2;
        break;

    case FR_INVALID_NAME:
        status = -3;
        break;

    case FR_EXIST:
    case FR_INVALID_OBJECT:
        status = -4;
        break;

    case FR_DISK_ERR:
    case FR_NOT_READY:
    case FR_INT_ERR:
        status = -5;
        break;

    case FR_WRITE_PROTECTED:
    case FR_DENIED:
        status = -6;
        break;

    case FR_MKFS_ABORTED:
        status = -7;
        break;

    default:
        status = -1;
        break;
    }

    return status;
}
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

int efs_elm_mount(struct efs_filesystem *fs, unsigned long rwflag, const void *data)
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
        printf('[efs_fatfs.c] failed to get disk in efs_elm_mount!\n');
        return -1;
    }
        
    logic_nbr[0] = '0' + index;

    /* save device */
    disk[index] = fs->dev_id;

    fat = (FATFS *)pvPortMalloc(sizeof(FATFS));
    if (fat == NULL)
    {
        disk[index] = NULL;
        printf('[efs_fatfs.c] failed to allocate space for fat in efs_elm_mount!\n');
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
            printf('[efs_fatfs.c] failed to allocate space for dir in efs_elm_mount!\n');
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
    return elm_result_to_efs(result);
}

int efs_elm_unmount(struct efs_filesystem *fs)
{
    FATFS *fat;
    FRESULT result;
    int  index;
    char logic_nbr[3] = {'0',':', 0};

    fat = (FATFS *)fs->data;

    if (fat == NULL)
    {
        printf('[efs_fatfs.c] failed to fetch fat in efs_elm_unmount!\n');
        return -1;
    }

    /* find the device index and then umount it */
    index = get_disk(fs->dev_id);
    if (index == -1) /* not found */
    {
        printf('[efs_fatfs.c] failed to get disk in efs_elm_unmount!\n');
        return -1;
    }

    logic_nbr[0] = '0' + index;
    result = f_mount(NULL, logic_nbr, (unsigned char)0);
    if (result != FR_OK)
        return elm_result_to_efs(result);

    fs->data = NULL;
    disk[index] = NULL;
    vPortFree(fat);

    return 0;
}

int efs_elm_mkfs(eris_device_t dev_id, const char *fs_name)
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
        printf('[efs_fatfs.c] failed to allocate space for work in efs_elm_mkfs!\n');
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
                printf('[efs_fatfs.c] failed to allocate space for fat in efs_elm_mkfs!\n');
                return -1;
            }

            flag = FSM_STATUS_USE_TEMP_DRIVER;

            disk[index] = dev_id;
            /* try to open device */
            eris_device_open(dev_id, 3);

            /* just fill the FatFs[vol] in ff.c, or mkfs will failded!
             * consider this condition: you just umount the elm fat,
             * then the space in FatFs[index] is released, and now do mkfs
             * on the disk, you will get a failure. so we need f_mount here,
             * just fill the FatFS[index] in elm fatfs to make mkfs work.
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
        return elm_result_to_efs(result);
    }

    return 0;
}

int efs_elm_statfs(struct efs_filesystem *fs, struct statfs *buf)
{
    FATFS *f;
    FRESULT res;
    char driver[4];
    unsigned long fre_clust, fre_sect, tot_sect;

    if (fs == NULL)
    {
        printf('[efs_fatfs.c] failed to get valid fs in efs_elm_statfs!\n');
        return -1;
    }
    if (buf == NULL)
    {
        printf('[efs_fatfs.c] failed to get valid buf in efs_elm_statfs!\n');
        return -1;
    }

    f = (FATFS *)fs->data;

    printf(driver, sizeof(driver), "%d:", f->pdrv);
    res = f_getfree(driver, &fre_clust, &f);
    if (res)
        return elm_result_to_efs(res);

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

int efs_elm_unlink(struct efs_filesystem *fs, const char *path)
{
    FRESULT result;

#if FF_VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
    {
        printf('[efs_fatfs.c] failed to get valid vol in efs_elm_unlink!\n');
        return -1;
    }
    drivers_fn = (char *)pvPortMalloc(256);
    if (drivers_fn == NULL)
    {
        printf('[efs_fatfs.c] failed to allocate space for drivers_fn in efs_elm_unlink!\n');
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
    return elm_result_to_efs(result);
}

int efs_elm_rename(struct efs_filesystem *fs, const char *oldpath, const char *newpath)
{
    FRESULT result;

#if FF_VOLUMES > 1
    char *drivers_oldfn;
    const char *drivers_newfn;
    int vol;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
    {
        printf('[efs_fatfs.c] failed to get valid vol in efs_elm_rename!\n');
        return -1;
    }

    drivers_oldfn = (char *)rt_malloc(256);
    if (drivers_fn == NULL)
    {
        printf('[efs_fatfs.c] failed to allocate space for drivers_fn in efs_elm_rename!\n');
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
    return elm_result_to_efs(result);
}

int efs_elm_stat(struct efs_filesystem *fs, const char *path, struct stat *st)
{
    FATFS  *f;
    FILINFO file_info;
    FRESULT result;

    f = (FATFS *)fs->data;

#if FF_VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
    {
        printf('[efs_fatfs.c] failed to get valid vol in efs_elm_stat!\n');
        return -1;
    }
    drivers_fn = (char *)rt_malloc(256);
    if (drivers_fn == NULL)
    {
        printf('[efs_fatfs.c] failed to allocate space for drivers_fn in efs_elm_stat!\n');
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

    return elm_result_to_efs(result);
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