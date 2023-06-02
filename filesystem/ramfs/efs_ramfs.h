#ifndef __EFS_RAMFS_H__
#define __EFS_RAMFS_H__

#include <efs_device.h>
#include <efs_file.h>
#include <efs_fs.h>
#include <efs.h>

#define RAMFS_NAME_MAX  32
#define RAMFS_MAGIC     0x0A0A0A0A


#define Eris_USING_MEMHEAP

struct ramfs_dirent
{
    eris_list_t list;
    struct efs_ramfs *fs;       /* file system ref */

    char name[RAMFS_NAME_MAX];  /* dirent name */
    eris_uint8_t *data;

    eris_size_t size;             /* file size */
};

struct efs_ramfs
{
    eris_uint32_t magic;

    struct eris_memheap memheap;
    struct ramfs_dirent root;
};

#endif