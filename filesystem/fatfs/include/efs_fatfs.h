#ifndef __EFS_FATFS_H__
#define __EFS_FATFS_H__

#include "headers.h"

int efs_fatfs_init(void);

static int get_disk(eris_device_t id);

int efs_fatfs_mount(struct efs_filesystem *fs, unsigned long rwflag, const void *data);

int efs_fatfs_unmount(struct efs_filesystem *fs);

int efs_fatfs_mkfs(eris_device_t dev_id, const char *fs_name);

int efs_fatfs_statfs(struct efs_filesystem *fs, struct statfs *buf);

int efs_fatfs_unlink(struct efs_filesystem *fs, const char *path);

int efs_fatfs_rename(struct efs_filesystem *fs, const char *oldpath, const char *newpath);

int efs_fatfs_stat(struct efs_filesystem *fs, const char *path, struct stat_efs *st);


int efs_fatfs_open(struct efs_file *file);

int efs_fatfs_close(struct efs_file *file);

int efs_fatfs_ioctl(struct efs_file *file, int cmd, void *args);

int efs_fatfs_read(struct efs_file *file, void *buf, size_t len);

int efs_fatfs_write(struct efs_file *file, const void *buf, size_t len);

// UNUSED
//int efs_fatfs_flush(struct efs_file *file);

int efs_fatfs_lseek(struct efs_file *file, off_t offset);

int efs_fatfs_getdents(struct efs_file *file, struct dirent *dirp, uint32_t count);


/* diskio */
DSTATUS disk_initialize(BYTE drv);

DSTATUS disk_status(BYTE drv);

DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, UINT count);

DRESULT disk_read(BYTE drv, BYTE *buff, DWORD sector, UINT count);

DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff);

DWORD get_fattime(void);

static struct efs_file_ops efs_fatfs_fops =
{
    efs_fatfs_open,
    efs_fatfs_close,
    efs_fatfs_ioctl,
    efs_fatfs_read,
    efs_fatfs_write,
    NULL, //flush - not used & unsupported
    efs_fatfs_lseek,
    efs_fatfs_getdents,
};
static struct efs_filesystem_ops efs_fatfs =
{
    "fatfs",
    0x00,
    &efs_fatfs_fops,

    efs_fatfs_mount,
    efs_fatfs_unmount,
    efs_fatfs_mkfs,
    efs_fatfs_statfs,

    efs_fatfs_unlink,
    efs_fatfs_stat,
    efs_fatfs_rename,
};

#endif
