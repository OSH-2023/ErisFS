#ifndef __EFS_RAMFS_H__
#define __EFS_RAMFS_H__

#include "headers.h"

#define RAMFS_NAME_MAX  32
#define RAMFS_MAGIC     0x0A0A0A0A


#define Eris_USING_MEMHEAP

struct ramfs_dirent
{
    eris_list_t list;
    struct efs_ramfs *fs;       /* file system ref */

    char name[RAMFS_NAME_MAX];  /* dirent name */
    uint8_t *data;

    size_t size;             /* file size */
};

struct eris_memheap a;

struct efs_ramfs
{
    uint32_t magic;

    struct eris_memheap memheap;
    struct ramfs_dirent root;
};

int efs_ramfs_init(void);

struct efs_ramfs *efs_ramfs_create(uint8_t *pool, size_t size);

#endif